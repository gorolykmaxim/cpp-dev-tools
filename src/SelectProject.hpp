#pragma once

#include "AppData.hpp"
#include "ChooseFile.hpp"

class SelectProject : public Process {
 public:
  SelectProject();
  void SanitizeProjectList(AppData& app);
  void LoadLastProjectOrDisplaySelectProjectView(AppData& app);
  void DisplaySelectProjectView(AppData& app);
  void OpenNewProject(AppData& app);
  void HandleOpenNewProjectCompletion(AppData& app);
  void OpenExistingProject(AppData& app);
  void FilterProjects(AppData& app);
  void RemoveProject(AppData& app);
  QList<QVariantList> MakeFilteredListOfProjects();

  QString filter;
  QList<Project> projects;
  QPtr<ChooseFile> choose_project;
};
