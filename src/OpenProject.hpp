#pragma once

#include "Application.hpp"

class OpenProject: public Process {
public:
  OpenProject();
  void DisplayOpenProjectView(Application& app);
};
