#pragma once

#include "AppData.hpp"

class SearchUserCommands: public Process {
public:
  SearchUserCommands();
  void DisplaySearchDialog(AppData& app);
  void Cancel(AppData& app);
};
