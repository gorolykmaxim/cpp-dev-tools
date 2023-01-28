#include "Initialize.hpp"

#include "ExecUserCmds.hpp"
#include "InitSqlDb.hpp"
#include "Process.hpp"
#include "SelectProject.hpp"

Initialize::Initialize() { EXEC_NEXT(ReadUserConfig); }

void Initialize::ReadUserConfig(AppData& app) {
  ScheduleProcess<InitSqlDb>(app, this);
  EXEC_NEXT(FinishInitAndSelectProject);
}

void Initialize::FinishInitAndSelectProject(AppData& app) {
  ScheduleProcess<SelectProject>(app, kViewSlot);
  ScheduleProcess<ExecUserCmds>(app, nullptr);
}
