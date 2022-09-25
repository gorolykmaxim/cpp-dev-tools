#pragma once

#include "Base.hpp"
#include "Application.hpp"
#include "JsonFileProcess.hpp"

class Initialize: public Process {
public:
  Initialize();
  void ReadConfig(Application& app);
  void LoadUserConfigAndOpenProject(Application& app);
private:
  QPtr<JsonFileProcess> read_config;
};
