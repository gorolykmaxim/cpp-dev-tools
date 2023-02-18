#pragma once

#include "AppData.hpp"

class DisplayExec : public Process {
 public:
  DisplayExec();
  void Display(AppData& app);
  void ReDrawExecHistory(AppData& app);
  void ReDrawExecOutput(AppData& app);
  void FilterExecs(AppData& app);
  void SelectExec(AppData& app);
  bool AutoReSelectExec(AppData& app);
  void UpdateAndDrawSelectedExec(AppData& app, bool only_update = false);
  QList<QVariantList> MakeFilteredListOfExecs(AppData& app);

  QString exec_filter;
  QUuid selected_exec_id;
  QString selected_exec_cmd;
  QString selected_exec_status;
  QString selected_exec_output;
  bool select_new_execs = true;
};
