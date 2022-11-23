#pragma once

#include "Lib.hpp"
#include "AppData.hpp"
#include "JsonFileProcess.hpp"

struct FileSuggestion {
  QString file;
  int match_start;
  int distance;
};

class OpenProject: public Process {
public:
  OpenProject();
  void DisplayOpenProjectView(AppData& app);
  void ChangeProjectPath(const QString& new_path, AppData& app);
  void HandleItemSelected(AppData& app, int item);
  void CreateNewProject(AppData& app);
  bool HasValidSuggestionAvailable() const;
  void LoadProjectFile(AppData& app);

  QString folder, file_name;
  int selected_suggestion = 0;
  QVector<FileSuggestion> suggestions;
  QPtr<Process> get_files;
  QPtr<JsonFileProcess> load_project_file;
};
