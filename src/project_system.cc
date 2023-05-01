#include "project_system.h"

#include <QDir>
#include <QStandardPaths>

#include "application.h"
#include "database.h"
#include "path.h"

#define LOG() qDebug() << "[ProjectSystem]"

QString Project::GetPathRelativeToHome() const {
  QString home_str =
      QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  if (path.startsWith(home_str)) {
    return "~/" + QDir(home_str).relativeFilePath(path);
  } else {
    return path;
  }
}

QString Project::GetFolderName() const { return Path::GetFileName(path); }

bool Project::IsNull() const { return id.isNull(); }

bool Project::operator==(const Project& other) const { return id == other.id; }

bool Project::operator!=(const Project& other) const {
  return !(*this == other);
}

Project ProjectSystem::ReadProjectFromSql(QSqlQuery& sql) {
  Project project;
  project.id = sql.value(0).toUuid();
  project.path = sql.value(1).toString();
  project.is_opened = sql.value(2).toBool();
  project.last_open_time = sql.value(3).toDateTime();
  return project;
}

void ProjectSystem::SetCurrentProject(Project project) {
  Application& app = Application::Get();
  if (project.IsNull()) {
    LOG() << "Closing project" << current_project.path;
    current_project = Project();
    Database::ExecCmdAsync("UPDATE project SET is_opened=false");
    emit currentProjectChanged();
    app.sqlite.SetSelectedFile(SqliteFile());
    app.task.KillAllTasks();
    app.task.ClearTasks();
    app.view.SetCurrentView("SelectProject.qml");
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
    app.sqlite.InitializeSelectedFile();
    app.view.SetCurrentView("TaskList.qml");
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
