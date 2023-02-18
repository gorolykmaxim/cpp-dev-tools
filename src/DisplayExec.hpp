#pragma once

#include "AppData.hpp"

struct ExecOutputSearchResult {
  int start = 0;
  int end = 0;
};

class DisplayExec : public Process {
 public:
  DisplayExec();
  void Display(AppData& app);
  void ReDrawExecHistory(AppData& app);
  void ReDrawExecOutput(AppData& app);
  void FilterExecs(AppData& app);
  void SelectExec(AppData& app);
  void ResetSearchResults(AppData& app);
  void SearchExecOutput(AppData& app);
  void SearchExecOutput(AppData& app, const QString& search_term);
  bool AutoReSelectExec(AppData& app);
  void UpdateAndDrawSelectedExec(AppData& app, bool only_update = false);
  QList<QVariantList> MakeFilteredListOfExecs(AppData& app);

  QString exec_filter;
  QUuid selected_exec_id;
  QString selected_exec_cmd;
  QString selected_exec_status;
  QString selected_exec_output;
  int current_search_result = 0;
  QList<ExecOutputSearchResult> search_results;
  bool select_new_execs = true;
};
