#pragma once

#include "AppData.hpp"
#include "OpenProject.hpp"
#include "LoadTaskConfig.hpp"

class SelectProject: public Process {
public:
  SelectProject();
  void DisplaySelectProjectView(AppData& app);
  void OpenNewProject(AppData& app);
  void HandleOpenNewProjectCompletion(AppData& app);
  void OpenExistingProject(AppData& app);
  void HandleOpenExistingProjectCompletion(AppData& app);

  QPtr<OpenProject> open_project;
  QPtr<LoadTaskConfig> load_project_file;
};
