#pragma once

#include "Base.hpp"
#include "Application.hpp"
#include "JsonFileProcess.hpp"

struct FileSuggestion {
  QString file;
  int match_start;
  int distance;
};

class OpenProject: public Process {
public:
  OpenProject();
  void DisplayOpenProjectView(Application& app);
  void ChangeProjectPath(const QString& new_path, Application& app);
  void HandleEnter(Application& app);
  void CreateNewProject(Application& app);
  bool HasValidSuggestionAvailable() const;
  void LoadProjectFile(Application& app);

  QString folder, file_name;
  int selected_suggestion = 0;
  QVector<FileSuggestion> suggestions;
  QPtr<Process> get_files;
  QPtr<JsonFileProcess> load_project_file;
};
