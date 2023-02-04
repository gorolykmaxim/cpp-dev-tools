#pragma once

#include "AppData.hpp"

class DisplayExec : public Process {
 public:
  DisplayExec();
  void Display(AppData& app);
  void ReDrawExecHistory(AppData& app);
  void FilterExecs(AppData& app);
  void SelectExec(AppData& app);
  QList<QVariantList> MakeFilteredListOfExecs(AppData& app);

  QString exec_filter;
  QUuid selected_exec_id;
};
