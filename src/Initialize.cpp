#include "Initialize.hpp"
#include "OpenProject.hpp"

Initialize::Initialize() {
  EXEC_NEXT(ReadConfig);
}

void Initialize::ReadConfig(Application& app) {
  read_config = app.runtime.Schedule<JsonFileProcess>(this,
                                                      JsonOperation::kRead,
                                                      app.user_config_path);
  EXEC_NEXT(LoadUserConfigAndOpenProject);
}

void Initialize::LoadUserConfigAndOpenProject(Application& app) {
  app.LoadFrom(read_config->json);
  app.runtime.Schedule<JsonFileProcess>(this, JsonOperation::kWrite,
                                        app.user_config_path, app.Save());
  app.runtime.Schedule<OpenProject>(nullptr);
}
