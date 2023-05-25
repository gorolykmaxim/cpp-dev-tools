#include "git_commit_controller.h"

#include "application.h"

GitCommitController::GitCommitController(QObject *parent)
    : QObject(parent), files(new ChangedFileListModel(this)) {
  Application::Get().view.SetWindowTitle("Git Commit");
}

ChangedFileListModel::ChangedFileListModel(QObject *parent)
    : TextListModel(parent) {}

QVariantList ChangedFileListModel::GetRow(int) const { return {}; }

int ChangedFileListModel::GetRowCount() const { return 0; }
