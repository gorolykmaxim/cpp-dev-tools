#pragma once

#include "AppData.hpp"
#include "ChooseTask.hpp"

class RunTask : public Process {
 public:
  RunTask();
  void ChooseTaskToRun(AppData& app);
  void RunChosenTask(AppData& app);

  QPtr<ChooseTask> choose_task;
};
