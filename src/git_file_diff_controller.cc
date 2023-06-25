#include "git_file_diff_controller.h"

#include "application.h"
#include "os_command.h"

#define LOG() qDebug() << "[GitFileDiffController]"

GitFileDiffController::GitFileDiffController(QObject *parent)
    : QObject(parent) {
  Application::Get().view.SetWindowTitle("Git File Diff");
}

QString GitFileDiffController::GetFile() const { return file_path; }

void GitFileDiffController::setBranchesToCompare(const QString &value) {
  branches_to_compare = value;
}

void GitFileDiffController::setFilePath(const QString &value) {
  file_path = value;
}

void GitFileDiffController::showDiff() {
  LOG() << "Finding diff for file:" << file_path
        << "between branches:" << branches_to_compare;
  emit fileChanged();
  QString error_msg = "Git: Failed to compute diff for " + file_path;
  OsCommand::Run("git", {"diff", branches_to_compare, "--", file_path}, "",
                 error_msg)
      .Then(this, [this, error_msg](OsProcess p) {
        if (p.exit_code == 0) {
          SetDiff(p.output, "");
        } else {
          SetDiff("", error_msg);
        }
      });
}

void GitFileDiffController::SetDiff(const QString &diff, const QString &error) {
  raw_diff = diff;
  diff_error = error;
  emit rawDiffChanged();
}
