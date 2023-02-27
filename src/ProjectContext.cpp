#include "ProjectContext.hpp"

#include "Database.hpp"
#include "Project.hpp"

#define LOG() qDebug() << "[ProjectContext]"

void ProjectContext::CloseProject() {
  LOG() << "Closing project" << current_project.path;
  current_project = Project();
  Database::ExecCmdAsync("UPDATE project SET is_opened=false");
  emit currentProjectChanged();
}
