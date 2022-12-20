#pragma once

#include "AppData.hpp"

class SearchUserCommands: public Process {
public:
  SearchUserCommands();
  void DisplaySearchDialog(AppData& app);
  void Cancel(AppData& app);
  void FilterCommands(AppData& app);
  QList<QVariantList> MakeFilteredListOfCommands(AppData& app);

  QString filter;
};
