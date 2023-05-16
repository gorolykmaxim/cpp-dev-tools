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
  SetEmptyListPlaceholder("No branches found");
}

const GitBranch* GitBranchListModel::GetSelected() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? nullptr : &list[i];
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

GitBranchListController::GitBranchListController(QObject* parent)
    : QObject(parent), branches(new GitBranchListModel(this)) {
  Application::Get().view.SetWindowTitle("Git Branches");
  QObject::connect(branches, &TextListModel::selectedItemChanged, this,
                   [this] { emit selectedBranchChanged(); });
  FindBranches();
}

bool GitBranchListController::IsLocalBranchSelected() const {
  if (const GitBranch* b = branches->GetSelected()) {
    return !b->is_remote;
  } else {
    return false;
  }
}

void GitBranchListController::deleteSelectedBranch(bool force) {
  const GitBranch* branch = branches->GetSelected();
  if (!branch) {
    return;
  }
  LOG() << "Deleting branch" << branch->name << "force:" << force;
  QString del_arg = force ? "-D" : "-d";
  auto delete_branch = new OsProcess("git", {"branch", del_arg, branch->name});
  delete_branch->error_title =
      "Git: Failed to delete branch '" + branch->name + '\'';
  delete_branch->success_title = "Git: Deleted branch '" + branch->name + '\'';
  QObject::connect(delete_branch, &OsProcess::finished, this,
                   &GitBranchListController::FindBranches);
  delete_branch->Run();
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

void GitBranchListController::FindBranches() {
  LOG() << "Querying list of branches";
  branches->list.clear();
  branches->SetPlaceholder("Looking for branches...");
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
                     [this, process, find_local, child_processes] {
                       (*child_processes)--;
                       if (process->exit_code == 0) {
                         if (find_local) {
                           ParseLocalBranches(process->output, branches->list);
                         } else {
                           ParseRemoteBranches(process->output, branches->list);
                         }
                       }
                       if (*child_processes == 0) {
                         std::sort(branches->list.begin(), branches->list.end(),
                                   Compare);
                         branches->Load(-1);
                         branches->SetPlaceholder();
                       }
                     });
    process->Run();
  }
}
