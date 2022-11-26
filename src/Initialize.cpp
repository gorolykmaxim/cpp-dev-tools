#include "Initialize.hpp"
#include "SelectProject.hpp"
#include "UserConfig.hpp"
#include "Process.hpp"

Initialize::Initialize() {
  EXEC_NEXT(ReadConfig);
}

void Initialize::ReadConfig(AppData& app) {
  read_config = ScheduleProcess<JsonFileProcess>(app, this,
                                                 JsonOperation::kRead,
                                                 app.user_config_path);
  EXEC_NEXT(LoadUserConfigAndOpenProject);
}

void Initialize::LoadUserConfigAndOpenProject(AppData& app) {
  ReadUserConfigFrom(app, read_config->json);
  SaveToUserConfig(app);
  ScheduleProcess<SelectProject>(app, nullptr);
}
