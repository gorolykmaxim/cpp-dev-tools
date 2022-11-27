#include "SaveUserConfig.hpp"
#include "Process.hpp"
#include "JsonFileProcess.hpp"

SaveUserConfig::SaveUserConfig() {
  EXEC_NEXT(Save);
}

void SaveUserConfig::Save(AppData& app) {
  QJsonObject json;
  QJsonArray projects_arr;
  for (const Project& project: app.projects) {
    QJsonObject project_obj;
    project_obj["path"] = project.path;
    project_obj["profile"] = project.profile;
    projects_arr.append(project_obj);
  }
  json["projects"] = projects_arr;
  ScheduleProcess<JsonFileProcess>(app, this, JsonOperation::kWrite,
                                   app.user_config_path, QJsonDocument(json));
}
