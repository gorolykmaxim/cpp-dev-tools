#include "new_git_branch_controller.h"

#include "application.h"

NewGitBranchController::NewGitBranchController(QObject* parent)
    : QObject(parent) {
  Application::Get().view.SetWindowTitle("New Git Branch");
}

void NewGitBranchController::createBranch(const QString& name) {
  Application& app = Application::Get();
  app.git.CreateBranch(name, branch_basis).Then(this, [this](OsProcess proc) {
    if (proc.exit_code == 0) {
      emit success();
    } else {
      error = proc.output;
      emit errorChanged();
    }
  });
}
