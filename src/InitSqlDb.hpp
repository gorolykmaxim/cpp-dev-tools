#pragma once

#include "AppData.hpp"

class InitSqlDb : public Process {
 public:
  InitSqlDb();
  void Run(AppData& app);
};
