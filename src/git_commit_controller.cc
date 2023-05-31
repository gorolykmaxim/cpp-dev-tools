#include "git_commit_controller.h"

#include <QQmlContext>

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "theme.h"

#define LOG() qDebug() << "[GitCommitController]"

static bool ReadBoolFromSql(QSqlQuery &sql) { return sql.value(0).toBool(); }

static QFont GetMonoFont() {
  Application &app = Application::Get();
  QQmlContext *ctx = app.qml_engine.rootContext();
  QString family = ctx->contextProperty("monoFontFamily").toString();
  int size = ctx->contextProperty("monoFontSize").toInt();
  return QFont(family, size);
}

GitCommitController::GitCommitController(QObject *parent)
    : QObject(parent),
      files(new ChangedFileListModel(this)),
      diff_width(80),
      formatter(new DiffFormatter(this)),
      is_side_by_side_diff(true),
      mono_font_metrics(GetMonoFont()) {
  // This re-runs git diff when user switches to a different changed file.
  connect(files, &TextListModel::selectedItemChanged, this, [this] {
    if (files->IsUpdating()) {
      return;
    }
    DiffSelectedFile();
  });
  // This re-runs git diff when changes list gets initially loaded, gets updated
  // by a modifying git command or gets manually reloaded by the user.
  connect(files, &TextListModel::preSelectCurrentIndex, this,
          &GitCommitController::DiffSelectedFile);
  Application::Get().view.SetWindowTitle("Git Commit");
  findChangedFiles();
  IoTask::Run<bool>(
      this,
      [] {
        return Database::ExecQueryAndRead<bool>(
                   "SELECT side_by_side_view FROM git_commit_context",
                   ReadBoolFromSql)
            .constFirst();
      },
      [this](bool result) {
        LOG() << "Loaded diff view setting. Side-by-side view:" << result;
        is_side_by_side_diff = result;
        RedrawDiff();
      });
}

bool GitCommitController::HasChanges() const { return !files->list.isEmpty(); }

int GitCommitController::CalcSideBarWidth() const {
  static const QString kDummyGitCommitHeader(50, 'a');
  static const Theme kTheme;
  return mono_font_metrics.horizontalAdvance(kDummyGitCommitHeader) +
         kTheme.kBasePadding;
}

bool GitCommitController::IsSelectedFileModified() const {
  return raw_git_diff_output.size() >= 2 &&
         raw_git_diff_output[1].startsWith("index");
}

void GitCommitController::findChangedFiles() {
  LOG() << "Looking for changed files";
  OsCommand::Run("git", {"status", "--porcelain=v1"}, "",
                 "Git: Failed to detect changes")
      .Then(this, [this](OsProcess p) {
        bool was_empty = files->list.isEmpty();
        files->list.clear();
        if (p.exit_code != 0) {
          files->SetPlaceholder("Failed to detect git changes", "red");
        } else {
          for (QString line : p.output.split('\n', Qt::SkipEmptyParts)) {
            ChangedFile f;
            f.is_staged = !line.startsWith(' ') && !line.startsWith('?');
            int status = f.is_staged ? 0 : 1;
            if (line[status] == '?' || line[status] == 'A') {
              f.status = ChangedFile::kNew;
            } else if (line[status] == 'M') {
              f.status = ChangedFile::kModified;
            } else if (line[status] == 'D') {
              f.status = ChangedFile::kDeleted;
            }
            f.path = line.sliced(3);
            files->list.append(f);
          }
        }
        std::sort(files->list.begin(), files->list.end(),
                  [](const ChangedFile &a, const ChangedFile &b) {
                    return a.path > b.path;
                  });
        LOG() << "Found" << files->list.size() << "changed files";
        files->Load(-1);
        if (files->list.isEmpty() != was_empty) {
          emit filesChanged();
        }
      });
}

void GitCommitController::toggleStagedSelectedFile() {
  const ChangedFile *f = files->GetSelected();
  if (!f) {
    return;
  }
  if (f->is_staged) {
    LOG() << "Unstaging" << f->path;
    ExecuteGitCommand({"restore", "--staged", f->path}, "",
                      "Git: Failed to unstage file");
  } else {
    LOG() << "Staging" << f->path;
    ExecuteGitCommand({"add", f->path}, "", "Git: Failed to stage file");
  }
}

void GitCommitController::resetSelectedFile() {
  const ChangedFile *f = files->GetSelected();
  if (!f) {
    return;
  }
  LOG() << "Reseting" << f->path;
  if (!f->is_staged && f->status == ChangedFile::kNew) {
    QString path = f->path;
    IoTask::Run(
        this, [path] { QFile(path).remove(); }, [this] { findChangedFiles(); });
  } else {
    ExecuteGitCommand({"checkout", "HEAD", "--", f->path}, "",
                      "Git: Failed to reset file");
  }
}

void GitCommitController::commit(const QString &msg, bool commit_all,
                                 bool amend) {
  LOG() << "Committing"
        << "all files:" << commit_all;
  QStringList args = {"commit", "-F", "-"};
  if (commit_all) {
    args.append("-a");
  }
  if (amend) {
    args.append("--amend");
  }
  ExecuteGitCommand(args, msg, "Git: Failed to commit")
      .Then(this, [this](OsProcess p) {
        if (p.exit_code == 0) {
          emit commitMessageChanged("");
        }
      });
}

void GitCommitController::loadLastCommitMessage() {
  LOG() << "Loading last commit message";
  OsCommand::Run("git", {"log", "-1", "--pretty=format:%B"}, "",
                 "Git: Failed to get last commit message")
      .Then(this, [this](OsProcess p) {
        if (p.exit_code == 0) {
          if (p.output.endsWith('\n')) {
            p.output.removeLast();
          }
          emit commitMessageChanged(p.output);
        }
      });
}

void GitCommitController::resizeDiff(int width) {
  int approx_char_width = mono_font_metrics.horizontalAdvance('a');
  // Only redraw the character width has changed
  if (std::abs(width - diff_width) > approx_char_width) {
    diff_width = width;
    RedrawDiff();
  }
}

void GitCommitController::toggleUnifiedDiff() {
  is_side_by_side_diff = !is_side_by_side_diff;
  LOG() << "Setting diff view to side-by-side:" << is_side_by_side_diff;
  Database::ExecCmdAsync("UPDATE git_commit_context SET side_by_side_view = ?",
                         {is_side_by_side_diff});
  RedrawDiff();
}

void GitCommitController::rollbackChunk(int pos) {
  const ChangedFile *f = files->GetSelected();
  if (!f) {
    return;
  }
  LOG() << "Rolling back git diff chunk at" << pos << "in" << f->path;
  QStringList input;
  for (TextSection chunk : diff_chunks) {
    if (pos >= chunk.start && pos < chunk.end) {
      input.append("y\n");
    } else {
      input.append("n\n");
    }
  }
  ExecuteGitCommand({"checkout", "-p", "--", f->path}, input.join(""),
                    "Git: Failed to rollback diff chunk");
}

void GitCommitController::openChunkInEditor(int pos) {
  const ChangedFile *f = files->GetSelected();
  if (!f) {
    return;
  }
  int diff_line = diff.sliced(0, pos).count('\n');
  int file_line = diff_line_to_file_line[diff_line];
  LOG() << "Opening git diff chunk at" << pos << "line" << file_line << "of"
        << f->path << "in editor";
  Application::Get().editor.OpenFile(f->path, file_line, -1);
}

Promise<OsProcess> GitCommitController::ExecuteGitCommand(
    const QStringList &args, const QString &input, const QString &error_title) {
  return OsCommand::Run("git", args, input, error_title)
      .Then<OsProcess>(this, [this](OsProcess proc) {
        if (proc.exit_code == 0) {
          findChangedFiles();
        }
        return Promise<OsProcess>(proc);
      });
}

void GitCommitController::DiffSelectedFile() {
  const ChangedFile *selected = files->GetSelected();
  if (!selected) {
    LOG() << "Clearing git diff view";
    raw_git_diff_output.clear();
    diff.clear();
    SetDiffError("");
    formatter->diff_line_flags.clear();
    diff_chunks.clear();
    diff_line_to_file_line.clear();
    emit selectedFileChanged();
    return;
  }
  QString path = selected->path;
  LOG() << "Will get git diff of" << path;
  OsCommand::Run("git", {"diff", "HEAD", "--", path}, "",
                 "Git: Failed to get git diff")
      .Then(this, [this, path](OsProcess p) {
        if (p.exit_code == 0) {
          raw_git_diff_output = p.output.split('\n', Qt::SkipEmptyParts);
          RedrawDiff();
          SetDiffError("");
        } else {
          SetDiffError("Failed to git diff '" + path + '\'');
        }
      });
}

static int ParseLineNumber(const QString &str, QChar start) {
  int i = str.indexOf(start) + 1;
  int j = str.indexOf(',', i);
  if (j < 0) {
    j = str.indexOf(' ', i);
  }
  return str.sliced(i, j - i).toInt();
}

static QString MakeLineNumber(int number, int max_chars) {
  QString result = number > -1 ? QString::number(number) : "";
  return ' ' + QString(max_chars - 2 - result.size(), ' ') + result + ' ';
}

static int AddDiffLine(QStringList line_numbers, const QStringList &to_write,
                       int line_length, QStringList &result) {
  static const QRegularExpression kWordBreakRegex("\\b");
  if (to_write.isEmpty()) {
    return 0;
  }
  if (line_numbers.size() < to_write.size()) {
    line_numbers = QStringList(to_write.size(), "");
  }
  int result_size = result.size();
  // Figure out what is the available length for each diff side in the
  // side-by-side view, while accounting for space required for line numbers,
  // that will be displayed in the same lines.
  QList<int> lengths;
  // Find positions of word breaks in each input string.
  QList<QList<int>> wbss;
  QList<int> wbs_its(to_write.size(), 0);
  for (int i = 0; i < to_write.size(); i++) {
    lengths.append(line_length / to_write.size() - line_numbers[i].size());
    const QString &str = to_write[i];
    QList<int> &wbs = wbss.emplaceBack();
    wbs.append(0);
    for (auto it = kWordBreakRegex.globalMatch(str); it.hasNext();) {
      wbs.append(it.next().capturedStart());
    }
    wbs.append(str.size());
  }
  // Write input lines into result.
  while (true) {
    QString line;
    for (int i = 0; i < to_write.size(); i++) {
      const QString &str = to_write[i];
      QString &line_number = line_numbers[i];
      int length = lengths[i];
      QList<int> &wbs = wbss[i];
      int &it = wbs_its[i];
      line += line_number;
      line_number = QString(line_number.size(), ' ');
      int start = wbs[it];
      int chars = 0;
      for (; it < wbs.size() - 1; it++) {
        if (wbs[it + 1] >= start + length) {
          if (chars == 0) {
            // We can't break current section by word because it would overflow
            // the line. Need to break by character while cutting everything
            // up until the next word break.
            chars = length;
            it++;
          }
          break;
        }
        chars = wbs[it + 1] - start;
      }
      if (chars > 0) {
        line += str.sliced(start, chars);
      }
      if (length - chars > 0) {
        line += QString(length - chars, ' ');
      }
    }
    if (!line.trimmed().isEmpty()) {
      result.append(line);
    } else {
      break;
    }
  }
  return result.size() - result_size;
}

void GitCommitController::RedrawDiff() {
  // Calculate how many chars will fit into diff_width
  int char_width = mono_font_metrics.horizontalAdvance('a');
  float error = (float)mono_font_metrics.horizontalAdvance(QString(50, 'a')) /
                char_width / 50;
  int max_chars = diff_width / (char_width * error);
  // Pre-compute line numbers for a diff
  QList<int> lns_b, lns_a;
  int ln_b = -1, ln_a = -1;
  for (const QString &line : raw_git_diff_output) {
    if (line.startsWith('-')) {
      lns_b.append(ln_b++);
      lns_a.append(-1);
    } else if (line.startsWith('+')) {
      lns_b.append(-1);
      lns_a.append(ln_a++);
    } else if (line.startsWith(' ')) {
      lns_b.append(ln_b++);
      lns_a.append(ln_a++);
    } else {
      if (line.startsWith("@@ ")) {
        ln_b = ParseLineNumber(line, '-');
        ln_a = ParseLineNumber(line, '+');
      }
      lns_b.append(-1);
      lns_a.append(-1);
    }
  }
  int mcln_b = QString::number(ln_b).size() + 2;
  int mcln_a = QString::number(ln_a).size() + 2;
  // Draw diff
  QString file_path;
  if (const ChangedFile *f = files->GetSelected()) {
    file_path = f->path;
  }
  formatter->syntax.DetectLanguageByFile(file_path);
  if (is_side_by_side_diff && IsSelectedFileModified()) {
    DrawSideBySideDiff(lns_b, lns_a, max_chars, mcln_b, mcln_a);
  } else {
    DrawUnifiedDiff(lns_b, lns_a, max_chars, std::max(mcln_b, mcln_a));
  }
  emit selectedFileChanged();
}

void GitCommitController::DrawSideBySideDiff(const QList<int> &lns_b,
                                             const QList<int> &lns_a,
                                             int max_chars, int mcln_b,
                                             int mcln_a) {
  QList<int> diff_line_flags;
  QStringList result;
  bool is_header = true;
  QStringList lb_b, lb_a;
  QList<int> lnb_b, lnb_a;
  diff_chunks.clear();
  diff_line_to_file_line.clear();
  for (int i = 0; i < raw_git_diff_output.size(); i++) {
    const QString &line = raw_git_diff_output[i];
    if (is_header || line.startsWith("@@ ")) {
      if (line.startsWith("@@ ")) {
        is_header = false;
        SavePreviousChunkAndStartNewOne(result, max_chars);
      }
      int lines_cnt = AddDiffLine({}, {line}, max_chars, result);
      diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kHeader));
      diff_line_to_file_line.append(QList<int>(lines_cnt, -1));
    } else if (line.startsWith('+')) {
      lnb_a.append(lns_a[i]);
      lb_a.append(line.sliced(1));
    } else if (line.startsWith('-')) {
      lnb_b.append(lns_b[i]);
      lb_b.append(line.sliced(1));
    }
    if (line.startsWith(' ') || i == raw_git_diff_output.size() - 1) {
      int max_buff_size = std::max(lb_b.size(), lb_a.size());
      for (int i = 0; i < max_buff_size; i++) {
        QString l_b, l_a;
        int ln_b = -1;
        int ln_a = -1;
        int flags = 0;
        if (i < lb_b.size()) {
          l_b = lb_b[i];
          ln_b = lnb_b[i];
          flags |= DiffLineType::kDeleted;
        }
        if (i < lb_a.size()) {
          l_a = lb_a[i];
          ln_a = lnb_a[i];
          flags |= DiffLineType::kAdded;
        }
        int lines_cnt = AddDiffLine(
            {MakeLineNumber(ln_b, mcln_b), MakeLineNumber(ln_a, mcln_a)},
            {l_b, l_a}, max_chars, result);
        diff_line_flags.append(QList<int>(lines_cnt, flags));
        diff_line_to_file_line.append(QList<int>(lines_cnt, ln_a));
      }
      lb_b.clear();
      lb_a.clear();
      lnb_b.clear();
      lnb_a.clear();
      if (line.startsWith(' ')) {
        QString l = line.sliced(1);
        const QString &ln_b = MakeLineNumber(lns_b[i], mcln_b);
        const QString &ln_a = MakeLineNumber(lns_a[i], mcln_a);
        int lines_cnt = AddDiffLine({ln_b, ln_a}, {l, l}, max_chars, result);
        diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kUnchanged));
        diff_line_to_file_line.append(QList<int>(lines_cnt, lns_a[i]));
      }
    }
  }
  SavePreviousChunkAndStartNewOne(result, max_chars, false);
  formatter->diff_line_flags = diff_line_flags;
  formatter->line_number_width_before = mcln_b;
  formatter->line_number_width_after = mcln_a;
  formatter->is_side_by_side_diff = true;
  diff = result.join('\n');
}

void GitCommitController::DrawUnifiedDiff(const QList<int> &lns_b,
                                          const QList<int> &lns_a,
                                          int max_chars, int mcln) {
  QList<int> diff_line_flags;
  QStringList result;
  bool is_header = true;
  diff_chunks.clear();
  diff_line_to_file_line.clear();
  for (int i = 0; i < raw_git_diff_output.size(); i++) {
    const QString &line = raw_git_diff_output[i];
    if (is_header || line.startsWith("@@ ")) {
      if (line.startsWith("@@ ")) {
        is_header = false;
        SavePreviousChunkAndStartNewOne(result, max_chars);
      }
      int lines_cnt = AddDiffLine({}, {line}, max_chars, result);
      diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kHeader));
      diff_line_to_file_line.append(QList<int>(lines_cnt, -1));
    } else if (line.startsWith('+')) {
      int lines_cnt = AddDiffLine({MakeLineNumber(lns_a[i], mcln)},
                                  {line.sliced(1)}, max_chars, result);
      diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kAdded));
      diff_line_to_file_line.append(QList<int>(lines_cnt, lns_a[i]));
    } else if (line.startsWith('-')) {
      int lines_cnt = AddDiffLine({MakeLineNumber(lns_b[i], mcln)},
                                  {line.sliced(1)}, max_chars, result);
      diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kDeleted));
      diff_line_to_file_line.append(QList<int>(lines_cnt, lns_a[i]));
    } else if (line.startsWith(' ')) {
      int lines_cnt = AddDiffLine({MakeLineNumber(lns_a[i], mcln)},
                                  {line.sliced(1)}, max_chars, result);
      diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kUnchanged));
      diff_line_to_file_line.append(QList<int>(lines_cnt, lns_a[i]));
    }
  }
  SavePreviousChunkAndStartNewOne(result, max_chars, false);
  formatter->diff_line_flags = diff_line_flags;
  formatter->line_number_width_before = mcln;
  formatter->line_number_width_after = 0;
  formatter->is_side_by_side_diff = false;
  diff = result.join('\n');
}

void GitCommitController::SetDiffError(const QString &text) {
  diff_error = text;
  emit diffErrorChanged();
}

void GitCommitController::SavePreviousChunkAndStartNewOne(
    const QStringList &result, int max_chars, bool start_new) {
  if (!diff_chunks.isEmpty()) {
    diff_chunks.last().end = result.size() * max_chars + result.size();
  }
  if (start_new) {
    diff_chunks.emplaceBack().start = result.size() * max_chars + result.size();
  }
}

ChangedFileListModel::ChangedFileListModel(QObject *parent)
    : TextListModel(parent) {
  SetRoleNames(
      {{0, "title"}, {1, "titleColor"}, {2, "icon"}, {3, "iconColor"}});
  searchable_roles = {0};
  SetEmptyListPlaceholder("No git changes found");
}

const ChangedFile *ChangedFileListModel::GetSelected() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? nullptr : &list[i];
}

QVariantList ChangedFileListModel::GetRow(int i) const {
  const ChangedFile &f = list[i];
  QString color;
  if (f.status == ChangedFile::kNew) {
    color = "lime";
  } else if (f.status == ChangedFile::kDeleted) {
    color = "red";
  }
  QString icon = "check_box_outline_blank";
  if (f.is_staged) {
    icon = "check_box";
  }
  return {f.path, color, icon, color};
}

int ChangedFileListModel::GetRowCount() const { return list.size(); }

DiffFormatter::DiffFormatter(QObject *parent)
    : TextAreaFormatter(parent),
      is_side_by_side_diff(true),
      line_number_width_before(0),
      line_number_width_after(0) {
  static const Theme kTheme;
  header_format.setForeground(ViewSystem::BrushFromHex(kTheme.kColorSubText));
  header_format.setBackground(ViewSystem::BrushFromHex(kTheme.kColorBgBlack));
  added_format.setBackground(ViewSystem::BrushFromHex("#4d3fb950"));
  added_placeholder_format.setBackground(ViewSystem::BrushFromHex("#262ea043"));
  deleted_format.setBackground(ViewSystem::BrushFromHex("#4df85149"));
  deleted_placeholder_format.setBackground(
      ViewSystem::BrushFromHex("#1af85149"));
  line_number_format.setForeground(
      ViewSystem::BrushFromHex(kTheme.kColorSubText));
}

QList<TextSectionFormat> DiffFormatter::Format(const QString &text,
                                               const QTextBlock &block) {
  QList<TextSectionFormat> fs;
  if (block.firstLineNumber() >= diff_line_flags.size()) {
    return fs;
  }
  int flags = diff_line_flags[block.firstLineNumber()];
  if (is_side_by_side_diff) {
    FormatSideBySide(text, block, flags, fs);
  } else {
    FormatUnified(text, block, flags, fs);
  }
  return fs;
}

void DiffFormatter::FormatSideBySide(const QString &text,
                                     const QTextBlock &block, int flags,
                                     QList<TextSectionFormat> &fs) {
  if (flags & DiffLineType::kHeader) {
    TextSectionFormat f;
    f.section.start = 0;
    f.section.end = text.size() - 1;
    f.format = header_format;
    fs.append(f);
  } else {
    QTextCharFormat *format;
    TextSectionFormat f;
    // Left side line number
    f.section.start = 0;
    f.section.end = line_number_width_before - 1;
    f.format = line_number_format;
    fs.append(f);
    // Left side diff
    format = nullptr;
    f.section.start = line_number_width_before;
    f.section.end = text.size() / 2 - 1;
    if (flags & DiffLineType::kDeleted) {
      format = &deleted_format;
    } else if (flags & DiffLineType::kAdded) {
      format = &deleted_placeholder_format;
    }
    if (format) {
      f.format = *format;
      fs.append(f);
    }
    ApplySyntaxFormatter(text, block, f.section, format, fs);
    // Right side line number
    f.section.start = text.size() / 2;
    f.section.end = text.size() / 2 + line_number_width_after - 1;
    f.format = line_number_format;
    fs.append(f);
    // Right side diff
    format = nullptr;
    f.section.start = text.size() / 2 + line_number_width_after;
    f.section.end = text.size() - 1;
    if (flags & DiffLineType::kAdded) {
      format = &added_format;
    } else if (flags & DiffLineType::kDeleted) {
      format = &added_placeholder_format;
    }
    if (format) {
      f.format = *format;
      fs.append(f);
    }
    ApplySyntaxFormatter(text, block, f.section, format, fs);
  }
}

void DiffFormatter::FormatUnified(const QString &text, const QTextBlock &block,
                                  int flags, QList<TextSectionFormat> &fs) {
  if (flags & DiffLineType::kHeader) {
    TextSectionFormat f;
    f.section.start = 0;
    f.section.end = text.size() - 1;
    f.format = header_format;
    fs.append(f);
  } else {
    TextSectionFormat f;
    // Line number
    f.section.start = 0;
    f.section.end = line_number_width_before - 1;
    f.format = line_number_format;
    fs.append(f);
    // Diff
    QTextCharFormat *format = nullptr;
    f.section.start = line_number_width_before;
    f.section.end = text.size() - 1;
    if (flags & DiffLineType::kDeleted) {
      format = &deleted_format;
    } else if (flags & DiffLineType::kAdded) {
      format = &added_format;
    }
    if (format) {
      f.format = *format;
      fs.append(f);
    }
    ApplySyntaxFormatter(text, block, f.section, format, fs);
  }
}

void DiffFormatter::ApplySyntaxFormatter(const QString &text,
                                         const QTextBlock &block,
                                         TextSection section,
                                         const QTextCharFormat *diff_format,
                                         QList<TextSectionFormat> &results) {
  QString str = text.sliced(section.start, section.end - section.start);
  QList<TextSectionFormat> fs = syntax.Format(str, block);
  for (TextSectionFormat &f : fs) {
    f.section.start += section.start;
    f.section.end += section.start;
    if (diff_format) {
      f.format.setBackground(diff_format->background());
    }
  }
  results.append(fs);
}
