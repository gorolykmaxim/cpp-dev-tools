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
  QString family = app.qml_engine.rootContext()
                       ->contextProperty("monoFontFamily")
                       .toString();
  int size =
      app.qml_engine.rootContext()->contextProperty("monoFontSize").toInt();
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
          diff.clear();
          formatter->diff_line_types.clear();
          emit selectedFileChanged();
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

static void AddDiffLine(const QString &before, const QString &after,
                        int max_line_width, QStringList &result) {
  result.append(before + QString(max_line_width - before.size(), ' ') + after +
                QString(max_line_width - after.size(), ' '));
}

void GitCommitController::DiffSelectedFile() {
  int i = files->GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  OsCommand::Run("git", {"diff", "HEAD", "--", files->list[i].path})
      .Then(this, [this](OsProcess p) {
        QStringList lines = p.output.split('\n', Qt::SkipEmptyParts);
        QList<DiffLineType> dlts(lines.size());
        qsizetype max_line_width = 80;
        for (int i = 0; i < lines.size(); i++) {
          QString &line = lines[i];
          if (line.startsWith("diff --git") || line.startsWith("index ") ||
              line.startsWith("deleted file mode") || line.startsWith("--- ") ||
              line.startsWith("+++ ") || line.startsWith("@@ ")) {
            dlts[i] = DiffLineType::kHeader;
          } else {
            if (line.startsWith('+')) {
              dlts[i] = DiffLineType::kAdded;
            } else if (line.startsWith('-')) {
              dlts[i] = DiffLineType::kDeleted;
            } else {
              dlts[i] = DiffLineType::kUnchanged;
            }
            line.removeFirst();
            max_line_width = std::max(line.size(), max_line_width);
          }
        }
        QStringList result;
        for (int i = 0; i < lines.size(); i++) {
          const QString &line = lines[i];
          switch (dlts[i]) {
            case DiffLineType::kHeader:
            case DiffLineType::kDeleted:
              AddDiffLine(line, "", max_line_width, result);
              break;
            case DiffLineType::kAdded:
              AddDiffLine("", line, max_line_width, result);
              break;
            case DiffLineType::kUnchanged:
              AddDiffLine(line, line, max_line_width, result);
              break;
            default:
              break;
          }
        }
        formatter->diff_line_types = dlts;
        diff = result.join('\n');
        emit selectedFileChanged();
      });
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
  if (block.firstLineNumber() >= diff_line_types.size()) {
    return fs;
  }
  switch (diff_line_types[block.firstLineNumber()]) {
    case DiffLineType::kHeader: {
      TextSectionFormat f;
      f.section.start = 0;
      f.section.end = text.size() - 1;
      f.format = header_format;
      fs.append(f);
      break;
    }
    case DiffLineType::kAdded: {
      TextSectionFormat f;
      f.section.start = 0;
      f.section.end = text.size() / 2 - 1;
      f.format = added_placeholder_format;
      fs.append(f);
      f.section.start = text.size() / 2;
      f.section.end = text.size() - 1;
      f.format = added_format;
      fs.append(f);
      break;
    }
    case DiffLineType::kDeleted: {
      TextSectionFormat f;
      f.section.start = 0;
      f.section.end = text.size() / 2 - 1;
      f.format = deleted_format;
      fs.append(f);
      f.section.start = text.size() / 2;
      f.section.end = text.size() - 1;
      f.format = deleted_placeholder_format;
      fs.append(f);
      break;
    }
    default:
      break;
  }
  return fs;
}
