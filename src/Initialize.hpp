#pragma once

#include "Lib.hpp"
#include "AppData.hpp"
#include "JsonFileProcess.hpp"

class Initialize: public Process {
public:
  Initialize();
  void ReadConfig(AppData& app);
  void LoadUserConfigAndOpenProject(AppData& app);
private:
  QPtr<JsonFileProcess> read_config;
};
