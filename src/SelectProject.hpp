#pragma once

#include "AppData.hpp"

class SelectProject: public Process {
public:
  SelectProject();
  void DisplaySelectProjectView(AppData& app);
};
