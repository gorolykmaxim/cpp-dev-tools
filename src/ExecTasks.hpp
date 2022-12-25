#pragma once

#include "AppData.hpp"

class ExecTasks: public Process {
public:
  ExecTasks();
  void DisplayExecTasksView(AppData& app);
  void FilterTasks(AppData& app);
  QList<QVariantList> MakeFilteredListOfTasks(AppData& app);

  QString taskFilter;
};
