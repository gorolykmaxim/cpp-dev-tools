#include "git_system.h"

#include <QDir>
#include <QProcess>
#include <QTextStream>

#include "application.h"
#include "os_command.h"

#define LOG() qDebug() << "[GitSystem]"

QList<QString> GitSystem::FindIgnoredPathsSync() {
  LOG() << "Looking for ignored files";
  QList<QString> results;
  QProcess proc;
  proc.start("git", QStringList() << "status"
                                  << "--porcelain=v1"
                                  << "--ignored");
  bool failed_to_start = !proc.waitForFinished();
  if (failed_to_start || proc.exitCode() != 0) {
    Notification notification("Git: Failed to find ignored files");
    notification.is_error = true;
    if (failed_to_start) {
      notification.description = "Failed to execute: " + proc.program() + ' ' +
                                 proc.arguments().join(' ');
    } else {
      notification.description = proc.readAllStandardError();
    }
    Application::Get().notification.Post(notification);
  } else {
    QTextStream stream(&proc);
    QDir folder = QDir::current();
    while (!stream.atEnd()) {
      QString line = stream.readLine().trimmed();
      QStringList row = line.split(' ', Qt::SkipEmptyParts);
      if (row.size() < 2 || row[0] != "!!") {
        continue;
      }
      results.append(folder.filePath(row[1]));
    }
  }
  return results;
}

void GitSystem::Pull() {
  Application::Get().notification.Post(Notification("Git: Pulling changes..."));
  ExecuteGitCommand({"pull", "origin", GetCurrentBranchName()},
                    "Git: Failed to pull changes", "Git: Changes pulled");
}

void GitSystem::Push() {
  Application::Get().notification.Post(Notification("Git: Pushing changes..."));
  OsCommand::Run("git", {"push", "origin", GetCurrentBranchName()}, "",
                 "Git: Failed to push changes", "Git: Changes pushed");
}

static void ParseLocalBranches(const QString& output,
                               QList<GitBranch>& branches) {
  for (QString line : output.split('\n', Qt::SkipEmptyParts)) {
    GitBranch b;
    b.is_current = line.startsWith('*');
    if (b.is_current) {
      line = line.sliced(1);
    }
    b.name = line.trimmed();
    branches.append(b);
  }
}

static void ParseRemoteBranches(const QString& output,
                                QList<GitBranch>& branches) {
  for (QString line : output.split('\n', Qt::SkipEmptyParts)) {
    line = line.trimmed();
    if (line.startsWith("origin/HEAD")) {
      continue;
    }
    GitBranch b;
    b.is_remote = true;
    b.name = line;
    branches.append(b);
  }
}

static bool Compare(const GitBranch& a, const GitBranch& b) {
  if (a.is_current != b.is_current) {
    return a.is_current > b.is_current;
  } else if (a.is_remote != b.is_remote) {
    return a.is_remote < b.is_remote;
  } else {
    return a.name < b.name;
  }
}

void GitSystem::FindBranches() {
  if (is_looking_for_branches) {
    return;
  }
  LOG() << "Querying list of branches";
  QString prev_name = GetCurrentBranchName();
  auto new_branches = QSharedPointer<QList<GitBranch>>::create();
  is_looking_for_branches = true;
  emit isLookingForBranchesChanged();
  Promise<void> local = OsCommand::Run("git", {"branch"}, "",
                                       "Git: Failed to find local branches")
                            .Then<void>(this, [new_branches](OsProcess proc) {
                              ParseLocalBranches(proc.output, *new_branches);
                              return QtFuture::makeReadyFuture();
                            });
  Promise<void> remote = OsCommand::Run("git", {"branch", "-r"}, "",
                                        "Git: Failed to find remote branches")
                             .Then<void>(this, [new_branches](OsProcess proc) {
                               ParseRemoteBranches(proc.output, *new_branches);
                               return QtFuture::makeReadyFuture();
                             });
  Promises::All<void>(this, local, remote)
      .Then(this, [new_branches, this, prev_name](QList<Promise<void>>) {
        LOG() << "Found" << new_branches->size() << "branches";
        std::sort(new_branches->begin(), new_branches->end(), Compare);
        branches = *new_branches;
        is_looking_for_branches = false;
        emit isLookingForBranchesChanged();
        if (prev_name != GetCurrentBranchName()) {
          emit currentBranchChanged();
        }
      });
}

void GitSystem::refreshBranchesIfProjectSelected() {
  if (!Application::Get().project.IsProjectOpened()) {
    return;
  }
  FindBranches();
}

Promise<OsProcess> GitSystem::CreateBranch(const QString& name,
                                           const QString& basis) {
  QStringList args = {"checkout", "-b", name};
  if (!basis.startsWith("(HEAD detached at")) {
    // Specifying such branch as basis will fail. At the same time in such case
    // we don't need to specify basis because we know that the specified
    // detached branch IS our current branch.
    args.append(basis);
  }
  return ExecuteGitCommand(args, "", "Git: Branch '" + name + "' created");
}

bool GitSystem::IsLookingForBranches() const { return is_looking_for_branches; }

const QList<GitBranch>& GitSystem::GetBranches() const { return branches; }

void GitSystem::ClearBranches() {
  LOG() << "Clearing list of branches";
  branches.clear();
  emit currentBranchChanged();
}

void GitSystem::checkoutBranch(int i) {
  static const QString kOriginPrefix = "origin/";
  if (i < 0 || i >= branches.size()) {
    return;
  }
  const GitBranch& branch = branches[i];
  LOG() << "Checking out branch" << branch.name;
  QStringList args;
  if (branch.is_remote) {
    QString local_name = branch.name;
    if (local_name.startsWith(kOriginPrefix)) {
      local_name.remove(0, kOriginPrefix.size());
    }
    args = {"checkout", "-b", local_name, branch.name};
  } else {
    args = {"checkout", branch.name};
  }
  ExecuteGitCommand(args,
                    "Git: Failed to checkout branch '" + branch.name + '\'',
                    "Git: Branch '" + branch.name + "\' checked-out");
}

void GitSystem::mergeBranchIntoCurrent(int i) {
  if (i < 0 || i >= branches.size()) {
    return;
  }
  const GitBranch& branch = branches[i];
  LOG() << "Merging branch" << branch.name << "into current";
  OsCommand::Run(
      "git", {"merge", branch.name}, "",
      "Git: Failed to merge branch '" + branch.name + "' into current",
      "Git: Branch '" + branch.name + "' is merged into current");
}

void GitSystem::deleteBranch(int i, bool force) {
  if (i < 0 || i >= branches.size()) {
    return;
  }
  const GitBranch& branch = branches[i];
  LOG() << "Deleting branch" << branch.name << "force:" << force;
  QStringList args = {"branch", branch.name};
  if (force) {
    args.insert(1, "-D");
  } else {
    args.insert(1, "-d");
  }
  ExecuteGitCommand(args, "Git: Failed to delete branch '" + branch.name + '\'',
                    "Git: Deleted branch '" + branch.name + '\'');
}

QString GitSystem::GetCurrentBranchName() const {
  for (const GitBranch& branch : branches) {
    if (branch.is_current) {
      return branch.name;
    }
  }
  return "";
}

Promise<OsProcess> GitSystem::ExecuteGitCommand(const QStringList& args,
                                                const QString& error,
                                                const QString& success) {
  return OsCommand::Run("git", args, "", error, success)
      .Then<OsProcess>(this, [this](OsProcess proc) {
        if (proc.exit_code == 0) {
          FindBranches();
        }
        return Promise<OsProcess>(proc);
      });
}
