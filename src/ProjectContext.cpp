#include "ProjectContext.hpp"

#include "Database.hpp"
#include "Project.hpp"

void ProjectContext::CloseProject() {
  current_project = Project();
  Database::ExecCmdAsync("UPDATE project SET is_opened=false");
  emit currentProjectChanged();
}
