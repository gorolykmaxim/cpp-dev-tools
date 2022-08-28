#pragma once

#include "ReadJsonFile.hpp"
#include "Application.hpp"
#include "Common.hpp"

class Initialize: public Process {
public:
  Initialize();
  void ReadConfig(Application& app);
  void LoadConfig(Application& app);
private:
  QPtr<ReadJsonFile> read_config;
};
