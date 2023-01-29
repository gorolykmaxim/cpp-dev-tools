#pragma once

#include "AppData.hpp"

struct FileSuggestion {
  QString file;
  int match_start;
  int distance;
};

class ChooseFile : public Process {
 public:
  ChooseFile();
  void DisplayChooseFileView(AppData& app);
  void ChangePath(AppData& app);
  void HandleItemSelected(AppData& app);
  void HandleOpenOrCreate(AppData& app);
  void AskUserWhetherToCreateTheFile(AppData& app);
  void CreateNew(AppData& app);
  QString MakeResult() const;

  QString window_title;
  bool choose_directory;
  QString folder;
  QString file_name;
  QString result;
  int selected_suggestion = 0;
  QList<FileSuggestion> suggestions;
  QPtr<Process> get_files;
};
