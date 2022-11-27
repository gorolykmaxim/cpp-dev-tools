#include "Initialize.hpp"
#include "SelectProject.hpp"
#include "Process.hpp"
#include "LoadUserConfig.hpp"
#include "SaveUserConfig.hpp"

Initialize::Initialize() {
  EXEC_NEXT(ReadUserConfig);
}

void Initialize::ReadUserConfig(AppData& app) {
  ScheduleProcess<LoadUserConfig>(app, this);
  EXEC_NEXT(UpdateConfigAndSelectProject);
}

void Initialize::UpdateConfigAndSelectProject(AppData& app) {
  ScheduleProcess<SaveUserConfig>(app, this);
  ScheduleProcess<SelectProject>(app, nullptr);
}
