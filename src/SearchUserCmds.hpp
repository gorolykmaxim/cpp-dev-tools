#pragma once

#include "AppData.hpp"

class SearchUserCmds: public Process {
public:
  SearchUserCmds();
  void DisplaySearchDialog(AppData& app);
  void Cancel(AppData& app);
  void FilterCommands(AppData& app);
  QList<QVariantList> MakeFilteredListOfCmds(AppData& app);

  QString filter;
};
