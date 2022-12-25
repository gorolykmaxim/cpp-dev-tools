#pragma once

#include "AppData.hpp"

class ExecUserCmds: public Process {
public:
  ExecUserCmds();
  void ListenToEvents(AppData& app);
  void Execute(AppData& app);
};
