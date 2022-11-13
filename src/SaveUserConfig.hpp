#pragma once

#include "Lib.hpp"
#include "Application.hpp"

class SaveUserConfig: public Process {
public:
  SaveUserConfig();
  void Run(Application& app);
};
