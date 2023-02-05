#pragma once

#include "AppData.hpp"

class ChooseTask : public Process {
 public:
  ChooseTask();
  void FindTasks(AppData& app);
  void Display(AppData& app);
  void FilterTasks(AppData& app);
  void HandleChosenTask(AppData& app);
  QList<QVariantList> MakeFilteredListOfTasks(AppData& app);

  QString window_title;
  QString filter;
  QList<QString> execs;
  QString result;
};
