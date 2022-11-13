#include "Initialize.hpp"
#include "OpenProject.hpp"
#include "SaveUserConfig.hpp"

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
  QJsonArray projects = read_config->json["projects"].toArray();
  for (QJsonValue project_val: projects) {
    Project project(project_val["path"].toString());
    project.profile = project_val["profile"].toInt();
    app.projects.append(project);
  }
  app.runtime.Schedule<SaveUserConfig>(nullptr);
  app.runtime.Schedule<OpenProject>(nullptr);
}
