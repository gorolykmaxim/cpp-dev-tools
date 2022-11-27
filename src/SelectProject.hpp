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
  void FilterProjects(AppData& app);
  QList<QVariantList> MakeFilteredListOfProjects(AppData& app);

  QString filter;
  QPtr<OpenProject> open_project;
  QPtr<LoadTaskConfig> load_project_file;
};
