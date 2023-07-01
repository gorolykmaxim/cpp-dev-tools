#include "run_cmake_controller.h"

#include "application.h"
#include "database.h"

#define LOG() qDebug() << "[RunCmakeController]"

RunCmakeController::RunCmakeController(QObject* parent) : QObject(parent) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Run CMake");
  Database::LoadState(this,
                      "SELECT source_folder, build_folder FROM cmake_context "
                      "WHERE project_id=?",
                      {app.project.GetCurrentProject().id},
                      [this](QVariantList result) {
                        if (result.isEmpty()) {
                          return;
                        }
                        source_folder = result[0].toString();
                        build_folder = result[1].toString();
                        emit foldersChanged();
                      });
}

QString RunCmakeController::GetRootFolder() const {
  return Application::Get().project.GetCurrentProject().path;
}

void RunCmakeController::displayChooseSourceFolder() {
  Application::Get().view.SetWindowTitle("Choose Source Folder");
  emit displayChooseSourceFolderView();
}

void RunCmakeController::displayChooseBuildFolder() {
  Application::Get().view.SetWindowTitle("Choose Build Folder");
  emit displayChooseBuildFolderView();
}

void RunCmakeController::setBuildFolder(QString path) {
  path.replace(GetRootFolder(), ".");
  build_folder = std::move(path);
  SaveState();
  emit foldersChanged();
  back();
}

void RunCmakeController::setSourceFolder(QString path) {
  path.replace(GetRootFolder(), ".");
  source_folder = std::move(path);
  SaveState();
  emit foldersChanged();
  back();
}

void RunCmakeController::back() {
  Application::Get().view.SetWindowTitle("Run CMake");
  emit displayView();
}

void RunCmakeController::runCmake() {
  LOG() << "Running CMake in build folder:" << build_folder
        << "and source folder:" << source_folder;
  CmakeTask t{source_folder, build_folder};
  Application::Get().task.RunTask(t.GetId(), t, false);
}

void RunCmakeController::SaveState() const {
  Database::ExecCmdAsync("INSERT OR REPLACE INTO cmake_context VALUES(?, ?, ?)",
                         {Application::Get().project.GetCurrentProject().id,
                          source_folder, build_folder});
}
