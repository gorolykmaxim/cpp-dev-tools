#include "ProjectSystem.hpp"

#include <QDir>

#include "Application.hpp"
#include "Database.hpp"
#include "Project.hpp"

#define LOG() qDebug() << "[ProjectSystem]"

void ProjectSystem::SetCurrentProject(Project project) {
  Application& app = Application::Get();
  if (project.IsNull()) {
    LOG() << "Closing project" << current_project.path;
    current_project = Project();
    Database::ExecCmdAsync("UPDATE project SET is_opened=false");
    emit currentProjectChanged();
    app.task_executor.KillAllTasks();
    app.view_controller.SetCurrentView("SelectProject.qml");
  } else {
    LOG() << "Changing current project to" << project.path;
    project.is_opened = true;
    project.last_open_time = QDateTime::currentDateTime();
    QDir::setCurrent(project.path);
    Database::ExecCmdAsync(
        "UPDATE project SET is_opened=?, last_open_time=? WHERE id=?",
        {project.is_opened, project.last_open_time, project.id});
    current_project = project;
    emit currentProjectChanged();
    app.view_controller.SetCurrentView("RunTask.qml");
  }
}

const Project& ProjectSystem::GetCurrentProject() const {
  return current_project;
}

QString ProjectSystem::GetCurrentProjectPathRelativeToHome() const {
  return current_project.GetPathRelativeToHome();
}

bool ProjectSystem::IsProjectOpened() const {
  return !current_project.IsNull();
}
