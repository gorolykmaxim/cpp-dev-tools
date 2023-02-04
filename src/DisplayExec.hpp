#pragma once

#include "AppData.hpp"

class DisplayExec : public Process {
 public:
  DisplayExec();
  void Display(AppData& app);
};
