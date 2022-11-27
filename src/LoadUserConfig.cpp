#include "LoadUserConfig.hpp"
#include "Process.hpp"

LoadUserConfig::LoadUserConfig() {
  EXEC_NEXT(Load);
}

void LoadUserConfig::Load(AppData& app) {
  load_file = ScheduleProcess<JsonFileProcess>(app, this, JsonOperation::kRead,
                                               app.user_config_path);
  EXEC_NEXT(Read);
}

void LoadUserConfig::Read(AppData& app) {
  for (QJsonValue project_val: load_file->json["projects"].toArray()) {
    Project project(project_val["path"].toString());
    project.profile = project_val["profile"].toInt();
    app.projects.append(project);
  }
  app.current_project_path = load_file->json["currentProjectPath"].toString();
}
