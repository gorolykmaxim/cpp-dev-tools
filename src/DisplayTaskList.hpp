#pragma once

#include "AppData.hpp"

class DisplayTaskList : public Process {
 public:
  DisplayTaskList();
  void FindTasks(AppData& app);
  void Display(AppData& app);
  void FilterTasks(AppData& app);
  void ExecSelectedTask(AppData& app);
  QList<QVariantList> MakeFilteredListOfTasks(AppData& app);

  QString filter;
  QList<QString> execs;
};
