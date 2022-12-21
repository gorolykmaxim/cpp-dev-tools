#pragma once

#include "AppData.hpp"

class CloseProject: public Process {
public:
  CloseProject();
  void Close(AppData& app);
};
