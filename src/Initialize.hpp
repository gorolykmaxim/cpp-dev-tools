#pragma once

#include "AppData.hpp"

class Initialize: public Process {
public:
  Initialize();
  void ReadUserConfig(AppData& app);
  void UpdateConfigAndSelectProject(AppData& app);
};
