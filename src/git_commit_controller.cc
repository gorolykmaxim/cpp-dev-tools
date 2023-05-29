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

static int AddDiffLine(const QString &before, const QString &after,
                       int line_length, QStringList &result) {
  int result_size = result.size();
  int half_line = line_length / 2;
  int lines_b = std::max((int)std::ceil((float)before.size() / half_line), 1);
  int lines_a = std::max((int)std::ceil((float)after.size() / half_line), 1);
  int chars_to_write = std::max(lines_b, lines_a) * half_line;
  int chars_written = 0;
  while (chars_to_write > chars_written) {
    QString line;
    for (const QString *str : {&before, &after}) {
      int chars = std::min((int)str->size() - chars_written, half_line);
      if (chars < 0) {
        chars = 0;
      }
      if (chars > 0) {
        line += str->sliced(chars_written, chars);
      }
      if (half_line - chars > 0) {
        line += QString(half_line - chars, ' ');
      }
    }
    chars_written += half_line;
    result.append(line);
  }
  return result.size() - result_size;
}

void GitCommitController::RedrawDiff() {
  Application &app = Application::Get();
  QQmlContext *ctx = app.qml_engine.rootContext();
  QString family = ctx->contextProperty("monoFontFamily").toString();
  int size = ctx->contextProperty("monoFontSize").toInt();
  QFontMetrics m(QFont(family, size));
  int char_width = m.horizontalAdvance('a');
  float error = (float)m.horizontalAdvance(QString(50, 'a')) / char_width / 50;
  int max_chars = diff_width / (char_width * error);
  QList<int> diff_line_flags;
  QStringList result;
  bool is_header = true;
  QStringList before_buff, after_buff;
  for (const QString &line : raw_git_diff_output) {
    if (is_header || line.startsWith("@@ ")) {
      int padding = std::max(0, max_chars - (int)line.size());
      result.append(line + QString(padding, ' '));
      diff_line_flags.append(DiffLineType::kHeader);
      is_header = !line.startsWith("@@ ");
    } else if (line.startsWith('+')) {
      after_buff.append(line.sliced(1));
    } else if (line.startsWith('-')) {
      before_buff.append(line.sliced(1));
    } else {
      int max_buff_size = std::max(before_buff.size(), after_buff.size());
      for (int i = 0; i < max_buff_size; i++) {
        QString before, after;
        int flags = 0;
        if (i < before_buff.size()) {
          before = before_buff[i];
          flags |= DiffLineType::kDeleted;
        }
        if (i < after_buff.size()) {
          after = after_buff[i];
          flags |= DiffLineType::kAdded;
        }
        int lines_cnt = AddDiffLine(before, after, max_chars, result);
        diff_line_flags.append(QList<int>(lines_cnt, flags));
      }
      before_buff.clear();
      after_buff.clear();
      QString line_ = line.sliced(1);
      int lines_cnt = AddDiffLine(line_, line_, max_chars, result);
      diff_line_flags.append(QList<int>(lines_cnt, DiffLineType::kUnchanged));
    }
  }
  formatter->diff_line_flags = diff_line_flags;
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

DiffFormatter::DiffFormatter(QObject *parent) : TextAreaFormatter(parent) {
  static const Theme kTheme;
  header_format.setForeground(FromHex(kTheme.kColorSubText));
  header_format.setBackground(FromHex(kTheme.kColorBgBlack));
  added_format.setBackground(FromHex("#4d3fb950"));
  added_placeholder_format.setBackground(FromHex("#262ea043"));
  deleted_format.setBackground(FromHex("#4df85149"));
  deleted_placeholder_format.setBackground(FromHex("#1af85149"));
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
  }
  if (flags & DiffLineType::kDeleted || flags & DiffLineType::kAdded) {
    TextSectionFormat f;
    f.section.start = 0;
    f.section.end = text.size() / 2 - 1;
    if (flags & DiffLineType::kDeleted) {
      f.format = deleted_format;
    } else {
      f.format = deleted_placeholder_format;
    }
    fs.append(f);
    f.section.start = text.size() / 2;
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
