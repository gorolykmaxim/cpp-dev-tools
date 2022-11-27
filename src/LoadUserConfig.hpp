#pragma once

#include "AppData.hpp"
#include "JsonFileProcess.hpp"

class LoadUserConfig: public Process {
public:
  LoadUserConfig();
  void Load(AppData& app);
  void Read(AppData& app);

  QPtr<JsonFileProcess> load_file;
};
