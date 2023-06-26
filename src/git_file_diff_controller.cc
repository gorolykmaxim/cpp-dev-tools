#include "git_file_diff_controller.h"

#include "application.h"
#include "database.h"
#include "os_command.h"

#define LOG() qDebug() << "[GitFileDiffController]"

GitFileDiffController::GitFileDiffController(QObject *parent)
    : QObject(parent) {
  Application &app = Application::Get();
  app.view.SetWindowTitle("Git File Diff");
  QUuid project_id = app.project.GetCurrentProject().id;
  Database::LoadState(this,
                      "SELECT branches_to_compare, file_path FROM "
                      "git_file_diff_context WHERE project_id=?",
                      {project_id}, [this](QVariantList result) {
                        if (result.isEmpty()) {
                          return;
                        }
                        branches_to_compare = result[0].toString();
                        file_path = result[1].toString();
                        emit optionsChanged();
                      });
}

QString GitFileDiffController::GetFile() const { return file_path; }

void GitFileDiffController::setBranchesToCompare(const QString &value) {
  branches_to_compare = value;
  SaveOptions();
}

void GitFileDiffController::setFilePath(const QString &value) {
  file_path = value;
  SaveOptions();
}

void GitFileDiffController::showDiff() {
  LOG() << "Finding diff for file:" << file_path
        << "between branches:" << branches_to_compare;
  QString error_msg = "Git: Failed to compute diff for " + file_path;
  OsCommand::Run("git", {"diff", branches_to_compare, "--", file_path}, "",
                 error_msg)
      .Then(this, [this, error_msg](OsProcess p) {
        if (p.exit_code == 0) {
          SetDiff(p.output, "");
        } else {
          SetDiff("", error_msg + ":\n" + p.output);
        }
      });
}

void GitFileDiffController::SetDiff(const QString &diff, const QString &error) {
  raw_diff = diff;
  diff_error = error;
  emit rawDiffChanged();
}

void GitFileDiffController::SaveOptions() const {
  QUuid project_id = Application::Get().project.GetCurrentProject().id;
  Database::ExecCmdAsync(
      "INSERT OR REPLACE INTO git_file_diff_context VALUES(?, ?, ?)",
      {project_id, branches_to_compare, file_path});
}
