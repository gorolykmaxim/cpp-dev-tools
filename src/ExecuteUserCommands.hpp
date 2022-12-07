#pragma once

#include "AppData.hpp"

class ExecuteUserCommands: public Process {
public:
  ExecuteUserCommands();
  void ListenToEvents(AppData& app);
  void Execute(AppData& app);
};
