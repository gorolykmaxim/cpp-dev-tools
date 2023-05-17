#include "new_git_branch_controller.h"

#include "application.h"

NewGitBranchController::NewGitBranchController(QObject* parent)
    : QObject(parent) {
  Application::Get().view.SetWindowTitle("New Git Branch");
}

void NewGitBranchController::createBranch(const QString& name) {
  Application& app = Application::Get();
  OsProcess* process = app.git.CreateBranch(name, branch_basis);
  QObject::connect(process, &OsProcess::finished, this, [this, process] {
    if (process->exit_code == 0) {
      emit success();
    } else {
      error = process->output;
      emit errorChanged();
    }
  });
}
