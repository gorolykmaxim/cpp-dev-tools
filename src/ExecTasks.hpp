#pragma once

#include "AppData.hpp"

class ExecTasks: public Process {
public:
  ExecTasks();
  void DisplayExecTasksView(AppData& app);
  void FilterTasks(AppData& app);
  void FilterExecs(AppData& app);
  void ExecSelectedTask(AppData& app);
  void ReDrawExecOutput(AppData& app);
  void ReDrawExecHistory(AppData& app);
  void ExecSelected(AppData& app);
  QList<QVariantList> MakeFilteredListOfTasks(AppData& app);
  QList<QVariantList> MakeFilteredListOfExecs(AppData& app);

  QString task_filter;
  QString exec_filter;
  QUuid selected_exec_id;
  bool select_new_execs = true;
};
