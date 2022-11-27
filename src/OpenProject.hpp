#pragma once

#include "AppData.hpp"
#include "LoadTaskConfig.hpp"

struct FileSuggestion {
  QString file;
  int match_start;
  int distance;
};

class OpenProject: public Process {
public:
  OpenProject();
  void DisplayOpenProjectView(AppData& app);
  void ChangeProjectPath(AppData& app);
  void HandleItemSelected(AppData& app);
  void CreateNewProject(AppData& app);
  bool HasValidSuggestionAvailable() const;
  void HandleOpeningCompletion(AppData& app);

  QString folder, file_name;
  int selected_suggestion = 0;
  QList<FileSuggestion> suggestions;
  QPtr<Process> get_files;
  QPtr<LoadTaskConfig> load_project_file;
  bool opened = false;
};
