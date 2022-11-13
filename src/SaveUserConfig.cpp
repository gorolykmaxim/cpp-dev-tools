#include "SaveUserConfig.hpp"
#include "JsonFileProcess.hpp"

SaveUserConfig::SaveUserConfig() {
  EXEC_NEXT(Run);
}

void SaveUserConfig::Run(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString user_config_path = home + "/.cpp-dev-tools.json";
  QJsonObject json;
  QJsonArray projects;
  for (const Project& project: app.projects) {
    QJsonObject project_obj;
    project_obj["path"] = project.path;
    project_obj["profile"] = project.profile;
    projects.append(project_obj);
  }
  json["projects"] = projects;
  app.runtime.Schedule<JsonFileProcess>(this, JsonOperation::kWrite,
                                        user_config_path, QJsonDocument(json));
}
