#include "ProjectContext.hpp"

#include "Database.hpp"
#include "Project.hpp"

#define LOG() qDebug() << "[ProjectContext]"

void ProjectContext::SetCurrentProject(const Project &project) {
  LOG() << "Changing current project to" << project.path;
  current_project = project;
  emit currentProjectChanged();
}

QString ProjectContext::GetCurrentProjectPathRelativeToHome() const {
  return current_project.GetPathRelativeToHome();
}

void ProjectContext::CloseProject() {
  LOG() << "Closing project" << current_project.path;
  current_project = Project();
  Database::ExecCmdAsync("UPDATE project SET is_opened=false");
  emit currentProjectChanged();
}
