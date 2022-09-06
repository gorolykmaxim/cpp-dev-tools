#include "Initialize.hpp"
#include "OpenProject.hpp"
#include <QStandardPaths>
#include <QString>
#include <QJsonDocument>
#include <QDebug>
#include <QVector>
#include <QVariant>

Initialize::Initialize() {
  EXEC_NEXT(ReadConfig);
}

void Initialize::ReadConfig(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString user_config_path = home + "/.cpp-dev-tools.json";
  read_config = app.runtime.Schedule<JsonFileProcess>(this,
                                                      JsonOperation::kRead,
                                                      user_config_path);
  EXEC_NEXT(LoadUserConfigAndOpenProject);
}

void Initialize::LoadUserConfigAndOpenProject(Application& app) {
  app.user_config.LoadFrom(read_config->json);
  app.runtime.Schedule<JsonFileProcess>(this, JsonOperation::kWrite,
                                        read_config->path,
                                        app.user_config.Save());
  app.runtime.Schedule<OpenProject>(this);
}
