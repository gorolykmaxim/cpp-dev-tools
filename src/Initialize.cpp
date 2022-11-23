#include "Initialize.hpp"
#include "OpenProject.hpp"
#include "UserConfig.hpp"
#include "Process.hpp"

Initialize::Initialize() {
  EXEC_NEXT(ReadConfig);
}

void Initialize::ReadConfig(Application& app) {
  read_config = ScheduleProcess<JsonFileProcess>(app, this,
                                                 JsonOperation::kRead,
                                                 app.user_config_path);
  EXEC_NEXT(LoadUserConfigAndOpenProject);
}

void Initialize::LoadUserConfigAndOpenProject(Application& app) {
  ReadUserConfigFrom(app, read_config->json);
  SaveToUserConfig(app);
  ScheduleProcess<OpenProject>(app, nullptr);
}
