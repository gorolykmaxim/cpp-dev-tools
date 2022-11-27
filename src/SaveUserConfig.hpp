#pragma once

#include "AppData.hpp"

class SaveUserConfig: public Process {
public:
  SaveUserConfig();
  void Save(AppData& app);
};
