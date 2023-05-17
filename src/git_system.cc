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
  ExecuteGitCommand({"pull"}, "Git: Failed to pull changes",
                    "Git: Changes pulled");
}

void GitSystem::Push() {
  Application::Get().notification.Post(Notification("Git: Pushing changes..."));
  OsCommand::Run("git push", "Git: Failed to push changes",
                 "Git: Changes pushed");
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
  LOG() << "Querying list of branches";
  QString prev_name = GetCurrentBranchName();
  branches.clear();
  is_looking_for_branches = true;
  emit isLookingForBranchesChanged();
  auto child_processes = QSharedPointer<int>::create(2);
  for (int i = 0; i < *child_processes; i++) {
    bool find_local = i == 0;
    OsProcess* process = new OsProcess("git", {"branch"});
    if (find_local) {
      process->error_title = "Git: Failed to find local branches";
    } else {
      process->args.append("-r");
      process->error_title = "Git: Failed to find remote branches";
    }
    QObject::connect(process, &OsProcess::finished, this,
                     [this, process, find_local, child_processes, prev_name] {
                       (*child_processes)--;
                       if (process->exit_code == 0) {
                         if (find_local) {
                           ParseLocalBranches(process->output, branches);
                         } else {
                           ParseRemoteBranches(process->output, branches);
                         }
                       }
                       if (*child_processes == 0) {
                         LOG() << "Found" << branches.size() << "branches";
                         std::sort(branches.begin(), branches.end(), Compare);
                         is_looking_for_branches = false;
                         emit isLookingForBranchesChanged();
                         if (prev_name != GetCurrentBranchName()) {
                           emit currentBranchChanged();
                         }
                       }
                     });
    process->Run();
  }
}

bool GitSystem::IsLookingForBranches() const { return is_looking_for_branches; }

const QList<GitBranch>& GitSystem::GetBranches() const { return branches; }

void GitSystem::ClearBranches() {
  LOG() << "Clearing list of branches";
  branches.clear();
  emit currentBranchChanged();
}

void GitSystem::CheckoutBranch(int i) {
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

void GitSystem::DeleteBranch(int i, bool force) {
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

void GitSystem::ExecuteGitCommand(const QStringList& args, const QString& error,
                                  const QString& success) {
  auto cmd = new OsProcess("git", args);
  cmd->error_title = error;
  cmd->success_title = success;
  QObject::connect(cmd, &OsProcess::finished, this, &GitSystem::FindBranches);
  cmd->Run();
}
