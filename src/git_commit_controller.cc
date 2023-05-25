#include "git_commit_controller.h"

#include "application.h"
#include "io_task.h"

#define LOG() qDebug() << "[GitCommitController]"

GitCommitController::GitCommitController(QObject *parent)
    : QObject(parent), files(new ChangedFileListModel(this)) {
  Application::Get().view.SetWindowTitle("Git Commit");
  findChangedFiles();
}

bool GitCommitController::HasChanges() const { return !files->list.isEmpty(); }

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
