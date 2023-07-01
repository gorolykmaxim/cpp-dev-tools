#include "run_cmake_controller.h"

#include "application.h"

#define LOG() qDebug() << "[RunCmakeController]"

RunCmakeController::RunCmakeController(QObject *parent) : QObject(parent) {
  Application::Get().view.SetWindowTitle("Run CMake");
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
  emit foldersChanged();
  back();
}

void RunCmakeController::setSourceFolder(QString path) {
  path.replace(GetRootFolder(), ".");
  source_folder = std::move(path);
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
