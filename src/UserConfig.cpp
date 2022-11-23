#include "UserConfig.hpp"
#include "JsonFileProcess.hpp"
#include "Process.hpp"

void ReadUserConfigFrom(Application& app, const QJsonDocument& json) {
  for (QJsonValue project_val: json["projects"].toArray()) {
    Project project(project_val["path"].toString());
    project.profile = project_val["profile"].toInt();
    app.projects.append(project);
  }
}

void SaveToUserConfig(Application& app) {
  QJsonObject json;
  QJsonArray projects_arr;
  for (const Project& project: app.projects) {
    QJsonObject project_obj;
    project_obj["path"] = project.path;
    project_obj["profile"] = project.profile;
    projects_arr.append(project_obj);
  }
  json["projects"] = projects_arr;
  ScheduleProcess<JsonFileProcess>(app, nullptr, JsonOperation::kWrite,
                                   app.user_config_path, QJsonDocument(json));
}
