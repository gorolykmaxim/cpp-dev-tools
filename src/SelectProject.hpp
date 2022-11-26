#pragma once

#include "AppData.hpp"
#include "OpenProject.hpp"

class SelectProject: public Process {
public:
  SelectProject();
  void DisplaySelectProjectView(AppData& app);
  void OpenNewProject(AppData& app);
  void HandleOpenProjectCompletion(AppData& app);

  QPtr<OpenProject> open_project;
};
