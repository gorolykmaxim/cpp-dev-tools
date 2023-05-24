#include "git_branch_list_controller.h"

#include <algorithm>

#include "application.h"
#include "theme.h"

#define LOG() qDebug() << "[GitBranchListController]"

GitBranchListModel::GitBranchListModel(QObject* parent)
    : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "titleColor"}, {2, "icon"}});
  searchable_roles = {0};
  SetEmptyListPlaceholder("No branches found");
  GitSystem& git = Application::Get().git;
  QObject::connect(&git, &GitSystem::isLookingForBranchesChanged, this,
                   [this, &git] {
                     if (git.IsLookingForBranches()) {
                       SetPlaceholder("Looking for branches...");
                     } else {
                       Load();
                       SetPlaceholder();
                     }
                   });
}

const GitBranch* GitBranchListModel::GetSelected() const {
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return nullptr;
  } else {
    return &Application::Get().git.GetBranches()[i];
  }
}

QVariantList GitBranchListModel::GetRow(int i) const {
  static const Theme kTheme;
  Application& app = Application::Get();
  const GitBranch& b = app.git.GetBranches()[i];
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

int GitBranchListModel::GetRowCount() const {
  return Application::Get().git.GetBranches().size();
}

GitBranchListController::GitBranchListController(QObject* parent)
    : QObject(parent), branches(new GitBranchListModel(this)) {
  QObject::connect(branches, &TextListModel::selectedItemChanged, this,
                   [this] { emit selectedBranchChanged(); });
}

bool GitBranchListController::IsLocalBranchSelected() const {
  if (const GitBranch* b = branches->GetSelected()) {
    return !b->is_remote;
  } else {
    return false;
  }
}

QString GitBranchListController::GetSelectedBranchName() const {
  if (const GitBranch* b = branches->GetSelected()) {
    return b->name;
  } else {
    return "";
  }
}

void GitBranchListController::deleteSelected(bool force) {
  Application::Get().git.DeleteBranch(branches->GetSelectedItemIndex(), force);
}

void GitBranchListController::checkoutSelected() {
  Application::Get().git.CheckoutBranch(branches->GetSelectedItemIndex());
}

void GitBranchListController::mergeSelected() {
  Application::Get().git.MergeBranchIntoCurrent(
      branches->GetSelectedItemIndex());
}

void GitBranchListController::displayList() {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Git Branches");
  app.git.FindBranches();
}
