#include "Initialize.hpp"
#include <QStandardPaths>
#include <QString>
#include <QJsonDocument>

Initialize::Initialize() {
  EXEC_NEXT(ReadConfig);
}

void Initialize::ReadConfig(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString user_config_path = home + "/.cpp-dev-tools.json";
  read_config = app.runtime.Schedule<JsonFileProcess>(this,
                                                      JsonOperation::kRead,
                                                      user_config_path);
  EXEC_NEXT(LoadConfig);
}

void Initialize::LoadConfig(Application& app) {
  app.user_config.LoadFrom(read_config->json);
  app.runtime.Schedule<JsonFileProcess>(this, JsonOperation::kWrite,
                                        read_config->path,
                                        app.user_config.Save());
}
