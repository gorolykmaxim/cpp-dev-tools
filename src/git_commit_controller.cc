#include "git_commit_controller.h"

#include <QFontMetrics>
#include <QQmlContext>

#include "application.h"
#include "io_task.h"
#include "theme.h"

#define LOG() qDebug() << "[GitCommitController]"

GitCommitController::GitCommitController(QObject *parent)
    : QObject(parent),
      files(new ChangedFileListModel(this)),
      diff_width(80),
      formatter(new DiffFormatter(this)) {
  connect(files, &TextListModel::selectedItemChanged, this,
          &GitCommitController::DiffSelectedFile);
  connect(files, &TextListModel::preSelectCurrentIndex, this,
          &GitCommitController::DiffSelectedFile);
  Application::Get().view.SetWindowTitle("Git Commit");
  findChangedFiles();
}

bool GitCommitController::HasChanges() const { return !files->list.isEmpty(); }

int GitCommitController::CalcSideBarWidth() const {
  static const QString kDummyGitCommitHeader(50, 'a');
  static const Theme kTheme;
  Application &app = Application::Get();
  QQmlContext *ctx = app.qml_engine.rootContext();
  QString family = ctx->contextProperty("monoFontFamily").toString();
  int size = ctx->contextProperty("monoFontSize").toInt();
  QFontMetrics m(QFont(family, size));
  return m.horizontalAdvance(kDummyGitCommitHeader) + kTheme.kBasePadding;
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
  diff_width = width;
  RedrawDiff();
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
  int i = files->GetSelectedItemIndex();
  if (i < 0) {
    diff.clear();
    formatter->diff_line_flags.clear();
    emit selectedFileChanged();
    return;
  }
  OsCommand::Run("git", {"diff", "HEAD", "--", files->list[i].path})
      .Then(this, [this](OsProcess p) {
        raw_git_diff_output = p.output.split('\n', Qt::SkipEmptyParts);
        RedrawDiff();
      });
}

static int ParseLineNumber(const QString &str, QChar start) {
  int i = str.indexOf(start) + 1;
  int j = str.indexOf(',', i);
  return str.sliced(i, j - i).toInt();
}

static int AddDiffLine(QStringList line_numbers, const QStringList &to_write,
                       int line_length, QStringList &result) {
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
  for (const QString &line_number : line_numbers) {
    lengths.append(line_length / to_write.size() - line_number.size());
  }
  // Strings that we are about to write might not fit into a single line in the
  // side-by-side diff thus we might need to wrap them effectively appending
  // more than one line to the result. To keep the sides of the diff
  // synchronized we need to wrap the input strings into the same number of
  // lines in the result. Here we decide into how many lines we will wrap those
  // strings.
  int lines_to_write = 0;
  for (int i = 0; i < to_write.size(); i++) {
    const QString &str = to_write[i];
    int length = lengths[i];
    int lines = std::ceil((float)str.size() / length);
    lines = std::max(lines, 1);
    lines_to_write = std::max(lines, lines_to_write);
  }
  // Write input strings into the result.
  while (true) {
    QString line;
    for (int i = 0; i < to_write.size(); i++) {
      const QString &str = to_write[i];
      QString &line_number = line_numbers[i];
      int length = lengths[i];
      line += line_number;
      line_number = QString(line_number.size(), ' ');
      int lines_written = result.size() - result_size;
      int str_offset = lines_written * length;
      int chars = std::min((int)str.size() - str_offset, length);
      chars = std::max(chars, 0);
      if (chars > 0) {
        line += str.sliced(str_offset, chars);
      }
      if (length - chars > 0) {
        line += QString(length - chars, ' ');
      }
    }
    result.append(line);
    if (result.size() - result_size == lines_to_write) {
      break;
    }
  }
  return result.size() - result_size;
}

void GitCommitController::RedrawDiff() {
  // Calculate how many chars will fit into diff_width
  Application &app = Application::Get();
  QQmlContext *ctx = app.qml_engine.rootContext();
  QString family = ctx->contextProperty("monoFontFamily").toString();
  int size = ctx->contextProperty("monoFontSize").toInt();
  QFontMetrics m(QFont(family, size));
  int char_width = m.horizontalAdvance('a');
  float error = (float)m.horizontalAdvance(QString(50, 'a')) / char_width / 50;
  int max_chars = diff_width / (char_width * error);
  // Pre-compute line numbers for a side-by-side diff
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
  QStringList line_numbers_before, line_numbers_after;
  QString line_number_placeholder_before(mcln_b, ' ');
  QString line_number_placeholder_after(mcln_a, ' ');
  for (int i = 0; i < lns_b.size(); i++) {
    QString &sln_b = line_numbers_before.emplaceBack();
    QString &sln_a = line_numbers_after.emplaceBack();
    if (lns_b[i] >= 0) {
      sln_b = ' ' + QString::number(lns_b[i]) + ' ';
      sln_b = QString(mcln_b - sln_b.size(), ' ') + sln_b;
    }
    if (lns_a[i] >= 0) {
      sln_a = ' ' + QString::number(lns_a[i]) + ' ';
      sln_a = QString(mcln_a - sln_a.size(), ' ') + sln_a;
    }
  }
  // Turn raw_git_diff_output into a side-by-side diff
  QList<int> diff_line_flags;
  QStringList result;
  bool is_header = true;
  QStringList lb_b, lb_a;
  QStringList lnb_b, lnb_a;
  for (int i = 0; i < raw_git_diff_output.size(); i++) {
    const QString &line = raw_git_diff_output[i];
    if (is_header || line.startsWith("@@ ")) {
      int lines_cnt = AddDiffLine({}, {line}, max_chars, result);
      diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kHeader));
      is_header = !line.startsWith("@@ ");
    } else if (line.startsWith('+')) {
      lnb_a.append(line_numbers_after[i]);
      lb_a.append(line.sliced(1));
    } else if (line.startsWith('-')) {
      lnb_b.append(line_numbers_before[i]);
      lb_b.append(line.sliced(1));
    } else {
      int max_buff_size = std::max(lb_b.size(), lb_a.size());
      for (int i = 0; i < max_buff_size; i++) {
        QString l_b, l_a;
        QString ln_b = line_number_placeholder_before;
        QString ln_a = line_number_placeholder_after;
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
        int lines_cnt =
            AddDiffLine({ln_b, ln_a}, {l_b, l_a}, max_chars, result);
        diff_line_flags.append(QList<int>(lines_cnt, flags));
      }
      lb_b.clear();
      lb_a.clear();
      lnb_b.clear();
      lnb_a.clear();
      QString l = line.sliced(1);
      const QString &ln_b = line_numbers_before[i];
      const QString &ln_a = line_numbers_after[i];
      int lines_cnt = AddDiffLine({ln_b, ln_a}, {l, l}, max_chars, result);
      diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kUnchanged));
    }
  }
  formatter->diff_line_flags = diff_line_flags;
  formatter->line_number_width_before = mcln_b;
  formatter->line_number_width_after = mcln_a;
  diff = result.join('\n');
  emit selectedFileChanged();
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

static QBrush FromHex(const QString &color) {
  return QBrush(QColor::fromString(color));
}

DiffFormatter::DiffFormatter(QObject *parent)
    : TextAreaFormatter(parent),
      line_number_width_before(0),
      line_number_width_after(0) {
  static const Theme kTheme;
  header_format.setForeground(FromHex(kTheme.kColorSubText));
  header_format.setBackground(FromHex(kTheme.kColorBgBlack));
  added_format.setBackground(FromHex("#4d3fb950"));
  added_placeholder_format.setBackground(FromHex("#262ea043"));
  deleted_format.setBackground(FromHex("#4df85149"));
  deleted_placeholder_format.setBackground(FromHex("#1af85149"));
  line_number_format.setForeground(FromHex(kTheme.kColorSubText));
}

QList<TextSectionFormat> DiffFormatter::Format(const QString &text,
                                               const QTextBlock &block) {
  QList<TextSectionFormat> fs;
  if (block.firstLineNumber() >= diff_line_flags.size()) {
    return fs;
  }
  int flags = diff_line_flags[block.firstLineNumber()];
  if (flags & DiffLineType::kHeader) {
    TextSectionFormat f;
    f.section.start = 0;
    f.section.end = text.size() - 1;
    f.format = header_format;
    fs.append(f);
  } else {
    TextSectionFormat f;
    f.section.start = 0;
    f.section.end = line_number_width_before - 1;
    f.format = line_number_format;
    fs.append(f);
    f.section.start = text.size() / 2;
    f.section.end = text.size() / 2 + line_number_width_after - 1;
    f.format = line_number_format;
    fs.append(f);
  }
  if (flags & DiffLineType::kDeleted || flags & DiffLineType::kAdded) {
    TextSectionFormat f;
    f.section.start = line_number_width_before;
    f.section.end = text.size() / 2 - 1;
    if (flags & DiffLineType::kDeleted) {
      f.format = deleted_format;
    } else {
      f.format = deleted_placeholder_format;
    }
    fs.append(f);
    f.section.start = text.size() / 2 + line_number_width_after;
    f.section.end = text.size() - 1;
    if (flags & DiffLineType::kAdded) {
      f.format = added_format;
    } else {
      f.format = added_placeholder_format;
    }
    fs.append(f);
  }
  return fs;
}
