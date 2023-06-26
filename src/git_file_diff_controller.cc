#include "git_file_diff_controller.h"

#include "application.h"
#include "database.h"
#include "os_command.h"

#define LOG() qDebug() << "[GitFileDiffController]"

GitFileDiffController::GitFileDiffController(QObject *parent)
    : QObject(parent), files(new GitFileDiffModel(this)) {
  // This re-runs git diff when user switches to a different file.
  connect(files, &TextListModel::selectedItemChanged, this, [this] {
    if (files->IsUpdating()) {
      return;
    }
    DiffSelectedFile();
  });
  // This re-runs git diff when changes list gets initially loaded.
  connect(files, &TextListModel::preSelectCurrentIndex, this,
          &GitFileDiffController::DiffSelectedFile);
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
  QString err =
      "Git: Failed to determine the list of files changed in " + file_path;
  OsCommand::Run("git",
                 {"diff", "--name-only", branches_to_compare, "--", file_path},
                 "", err)
      .Then(this, [this, err](OsProcess p) {
        files->files.clear();
        if (p.exit_code == 0) {
          files->files = p.output.split('\n', Qt::SkipEmptyParts);
        } else {
          SetDiff("", err + ": " + p.output);
        }
        files->Load();
        emit files->filesChanged();
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

void GitFileDiffController::DiffSelectedFile() {
  QString file = files->GetSelectedFile();
  if (file.isEmpty()) {
    SetDiff("", diff_error);
    return;
  }
  QString err = "Git: Failed to compute diff for " + file;
  OsCommand::Run("git", {"diff", branches_to_compare, "--", file}, "", err)
      .Then(this, [this, err](OsProcess p) {
        if (p.exit_code == 0) {
          SetDiff(p.output, "");
        } else {
          SetDiff("", err + ":\n" + p.output);
        }
      });
}

GitFileDiffModel::GitFileDiffModel(QObject *parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}});
}

bool GitFileDiffModel::HasMoreThanOne() const { return files.size() > 1; }

QString GitFileDiffModel::GetSelectedFile() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? "" : files[i];
}

QVariantList GitFileDiffModel::GetRow(int i) const { return {files[i]}; }

int GitFileDiffModel::GetRowCount() const { return files.size(); }
