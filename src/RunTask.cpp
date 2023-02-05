#include "RunTask.hpp"

#include "DisplayExec.hpp"
#include "ExecTask.hpp"
#include "Process.hpp"

RunTask::RunTask() { EXEC_NEXT(ChooseTaskToRun); }

void RunTask::ChooseTaskToRun(AppData &app) {
  choose_task = ScheduleProcess<ChooseTask>(app, this);
  choose_task->window_title = "Run Task";
  EXEC_NEXT(RunChosenTask);
}

void RunTask::RunChosenTask(AppData &app) {
  ScheduleProcess<ExecTask>(app, nullptr, choose_task->result);
  ScheduleProcess<DisplayExec>(app, kViewSlot);
}
