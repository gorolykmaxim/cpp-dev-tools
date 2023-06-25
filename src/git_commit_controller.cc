#include "git_commit_controller.h"

#include <QQmlContext>

#include "application.h"
#include "io_task.h"
#include "theme.h"

#define LOG() qDebug() << "[GitCommitController]"

GitCommitController::GitCommitController(QObject *parent)
    : QObject(parent),
      files(new ChangedFileListModel(this)),
      formatter(new CommitMessageFormatter(this)) {
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
}

bool GitCommitController::HasChanges() const { return !files->list.isEmpty(); }

int GitCommitController::CalcSideBarWidth() const {
  static const Theme kTheme;
  QString kDummyGitCommitHeader(50, 'a');
  Application &app = Application::Get();
  QQmlContext *ctx = app.qml_engine.rootContext();
  QString family = ctx->contextProperty("monoFontFamily").toString();
  int size = ctx->contextProperty("monoFontSize").toInt();
  QFont font(family, size);
  QFontMetrics metrics(font);
  return metrics.horizontalAdvance(kDummyGitCommitHeader) + kTheme.kBasePadding;
}

void GitCommitController::findChangedFiles() {
  LOG() << "Looking for changed files";
  OsCommand::Run("git", {"status", "--porcelain=v1"}, "",
                 "Git: Failed to detect changes")
      .Then<OsProcess>(
          this,
          [this](OsProcess p) {
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
            if (files->list.isEmpty() != was_empty) {
              emit filesChanged();
            }
            return OsCommand::Run("git", {"diff", "HEAD", "--numstat"}, "",
                                  "Git: Failed to collect diff statistics");
          })
      .Then(this, [this](OsProcess p) {
        if (p.exit_code != 0) {
          files->SetPlaceholder("Failed to collect git diff statistics", "red");
        } else {
          for (const QString &line : p.output.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split('\t', Qt::SkipEmptyParts);
            QString file = parts[2];
            for (ChangedFile &f : files->list) {
              if (f.path == file) {
                f.additions = parts[0].toInt();
                f.removals = parts[1].toInt();
                break;
              }
            }
          }
          // Count added lines for unstaged files manually because git diff
          // won't tell about them.
          QList<QString> untracked_files;
          for (ChangedFile &f : files->list) {
            if (!f.is_staged && f.status == ChangedFile::kNew) {
              untracked_files.append(f.path);
            }
          }
          IoTask::Run<QHash<QString, int>>(
              this,
              [untracked_files] {
                QHash<QString, int> additions;
                for (const QString &path : untracked_files) {
                  QFile file(path);
                  if (!file.open(QIODevice::ReadOnly)) {
                    additions[path] = 0;
                  } else {
                    additions[path] = QString(file.readAll()).count('\n');
                  }
                }
                return additions;
              },
              [this](QHash<QString, int> additions) {
                for (ChangedFile &f : files->list) {
                  if (additions.contains(f.path)) {
                    f.additions = additions[f.path];
                  }
                }
                emit files->statsChanged();
                files->Load(-1);
              });
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

void GitCommitController::rollbackChunk(int chunk, int chunk_count) {
  const ChangedFile *f = files->GetSelected();
  if (!f) {
    return;
  }
  LOG() << "Rolling back git diff chunk " << chunk << "of" << chunk_count
        << "in" << f->path;
  QStringList input;
  for (int i = 0; i < chunk_count; i++) {
    if (chunk == i) {
      input.append("y\n");
    } else {
      input.append("n\n");
    }
  }
  ExecuteGitCommand({"checkout", "-p", "--", f->path}, input.join(""),
                    "Git: Failed to rollback diff chunk");
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
  static const QString kErrorTitle = "Git: Failed to get git diff";
  const ChangedFile *selected = files->GetSelected();
  if (!selected) {
    LOG() << "Clearing git diff view";
    SetDiff("", "");
    return;
  }
  QString path = selected->path;
  LOG() << "Will get git diff of" << path;
  Promise<OsProcess> git_diff;
  if (selected->status == ChangedFile::kNew && !selected->is_staged) {
    git_diff =
        OsCommand::Run("git", {"diff", "--no-index", "--", "/dev/null", path},
                       "", kErrorTitle, "", 1)
            .Then<OsProcess>(this, [](OsProcess p) {
              p.exit_code = p.exit_code == 1 ? 0 : p.exit_code;
              return Promise<OsProcess>(p);
            });
  } else {
    git_diff =
        OsCommand::Run("git", {"diff", "HEAD", "--", path}, "", kErrorTitle);
  }
  git_diff.Then(this, [this, path](OsProcess p) {
    if (p.exit_code == 0) {
      SetDiff(p.output, "");
    } else {
      SetDiff("", "Failed to git diff '" + path + '\'');
    }
  });
}

void GitCommitController::SetDiff(const QString &diff, const QString &error) {
  raw_diff = diff;
  emit rawDiffChanged();
  diff_error = error;
  emit diffErrorChanged();
}

ChangedFileListModel::ChangedFileListModel(QObject *parent)
    : TextListModel(parent) {
  SetRoleNames({
      {0, "title"},
      {1, "titleColor"},
      {2, "icon"},
      {3, "iconColor"},
      {4, "rightText"},
  });
  searchable_roles = {0};
  SetEmptyListPlaceholder("No git changes found");
}

const ChangedFile *ChangedFileListModel::GetSelected() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? nullptr : &list[i];
}

QString ChangedFileListModel::GetSelectedFileName() const {
  const ChangedFile *file = GetSelected();
  return file ? file->path : "";
}

QString ChangedFileListModel::GetStats() const {
  int additions = 0;
  int removals = 0;
  for (const ChangedFile &f : list) {
    additions += f.additions;
    removals += f.removals;
  }
  return FormatStats(additions, removals);
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
  return {f.path, color, icon, color, FormatStats(f.additions, f.removals)};
}

int ChangedFileListModel::GetRowCount() const { return list.size(); }

QString ChangedFileListModel::FormatStats(int additions, int removals) {
  QStringList stats;
  if (additions > 0) {
    stats.append('+' + QString::number(additions));
  }
  if (removals > 0) {
    stats.append('-' + QString::number(removals));
  }
  return stats.join(", ");
}

CommitMessageFormatter::CommitMessageFormatter(QObject *parent)
    : TextFormatter(parent) {
  format.setBackground(QBrush(Qt::red));
}

QList<TextFormat> CommitMessageFormatter::Format(const QString &text,
                                                 LineInfo line) const {
  QList<TextFormat> fs;
  if (line.number == 0 && text.size() >= 50) {
    FormatSuffixAfter(50, text.size(), fs);
  } else if (line.number == 1 && text.size() >= 1) {
    FormatSuffixAfter(1, text.size(), fs);
  } else if (line.number > 1 && text.size() >= 72) {
    FormatSuffixAfter(72, text.size(), fs);
  }
  return fs;
}

void CommitMessageFormatter::FormatSuffixAfter(int offset, int length,
                                               QList<TextFormat> &fs) const {
  TextFormat f;
  f.offset = offset - 1;
  f.length = length - f.offset;
  f.format = format;
  fs.append(f);
}
