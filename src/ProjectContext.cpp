#include "ProjectContext.hpp"

#include "Database.hpp"
#include "Project.hpp"

#define LOG() qDebug() << "[ProjectContext]"

void ProjectContext::SetCurrentProject(const Project &project) {
  if (project.IsNull()) {
    LOG() << "Closing project" << current_project.path;
    current_project = Project();
    Database::ExecCmdAsync("UPDATE project SET is_opened=false");
    emit currentProjectChanged();
  } else {
    LOG() << "Changing current project to" << project.path;
    current_project = project;
    emit currentProjectChanged();
  }
}

Project ProjectContext::GetCurrentProject() const { return current_project; }

QString ProjectContext::GetCurrentProjectPathRelativeToHome() const {
  return current_project.GetPathRelativeToHome();
}

bool ProjectContext::IsProjectOpened() const {
  return !current_project.IsNull();
}
