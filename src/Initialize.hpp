#pragma once

#include "AppData.hpp"

class Initialize : public Process {
 public:
  Initialize();
  void ReadUserConfig(AppData& app);
  void FinishInitAndSelectProject(AppData& app);
};
