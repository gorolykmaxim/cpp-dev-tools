#pragma once

#include "JsonFileProcess.hpp"
#include "Application.hpp"
#include "Common.hpp"

class Initialize: public Process {
public:
  Initialize();
  void ReadConfig(Application& app);
  void LoadConfig(Application& app);
  void DisplayUi(Application& app);
private:
  QPtr<JsonFileProcess> read_config;
};
