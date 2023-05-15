#include "git_branch_list_controller.h"

#include <algorithm>

#include "application.h"
#include "os_command.h"
#include "theme.h"

#define LOG() qDebug() << "[GitBranchListController]"

GitBranchListModel::GitBranchListModel(QObject* parent)
    : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "titleColor"}, {2, "icon"}});
  searchable_roles = {0};
}

QVariantList GitBranchListModel::GetRow(int i) const {
  static const Theme kTheme;
  const GitBranch& b = list[i];
  QString title = b.name;
  QString title_color;
  if (b.is_remote) {
    title_color = kTheme.kColorSubText;
  }
  if (b.is_current) {
    title = "[Current] " + title;
  }
  return {title, title_color, "call_split"};
}

int GitBranchListModel::GetRowCount() const { return list.size(); }

static bool Compare(const GitBranch& a, const GitBranch& b) {
  if (a.is_current != b.is_current) {
    return a.is_current > b.is_current;
  } else if (a.is_remote != b.is_remote) {
    return a.is_remote < b.is_remote;
  } else {
    return a.name < b.name;
  }
}

GitBranchListController::GitBranchListController(QObject* parent)
    : QObject(parent), branches(new GitBranchListModel(this)) {
  Application::Get().view.SetWindowTitle("Git Branches");
  auto find = new FindGitBranches(this);
  QObject::connect(find, &FindGitBranches::finished, this, [this, find] {
    branches->list = find->branches;
    std::sort(branches->list.begin(), branches->list.end(), Compare);
    branches->Load();
    SetIsLoading(false);
  });
  SetIsLoading(true);
  find->Run();
}

bool GitBranchListController::ShouldShowPlaceholder() const {
  return is_loading || branches->list.isEmpty();
}

void GitBranchListController::SetIsLoading(bool value) {
  is_loading = value;
  emit isLoadingChanged();
}

FindGitBranches::FindGitBranches(QObject* parent) : QObject(parent) {}

void FindGitBranches::Run() {
  LOG() << "Querying list of branches";
  child_processes = 2;
  for (int i = 0; i < child_processes; i++) {
    bool find_local = i == 0;
    OsProcess* process = new OsProcess("git", {"branch"});
    if (find_local) {
      process->error_title = "Git: Failed to find local branches";
    } else {
      process->args.append("-r");
      process->error_title = "Git: Failed to find remote branches";
    }
    QObject::connect(process, &OsProcess::finished, this,
                     [this, process, find_local] {
                       child_processes--;
                       if (process->exit_code == 0) {
                         if (find_local) {
                           ParseLocalBranches(process->output);
                         } else {
                           ParseRemoteBranches(process->output);
                         }
                       }
                       if (child_processes == 0) {
                         emit finished();
                         deleteLater();
                       }
                     });
    process->Run();
  }
}

void FindGitBranches::ParseLocalBranches(const QString& output) {
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

void FindGitBranches::ParseRemoteBranches(const QString& output) {
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
