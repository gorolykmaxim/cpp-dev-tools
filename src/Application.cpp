#include "Application.hpp"
#include "JsonFileProcess.hpp"

QString& Profile::operator[](const QString& key) {
  return variables[key];
}

QString Profile::operator[](const QString& key) const {
  return variables[key];
}

QString Profile::GetName() const {
  return variables["name"];
}

QList<QString> Profile::GetVariableNames() const {
  return variables.keys();
}

bool Profile::Contains(const QString& key) const {
  return variables.contains(key);
}

Project::Project(const QString& path) : path(path), profile(-1) {}

bool Project::operator==(const Project& project) const {
  return path == project.path;
}

bool Project::operator!=(const Project& project) const {
  return !(*this == project);
}

Application::Application(int argc, char** argv)
    : gui_app(argc, argv),
      runtime(*this),
      threads(gui_app),
      ui() {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  user_config_path = home + "/.cpp-dev-tools.json";
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
}

void Application::LoadFrom(const QJsonDocument& json) {
  for (QJsonValue project_val: json["projects"].toArray()) {
    Project project(project_val["path"].toString());
    project.profile = project_val["profile"].toInt();
    projects.append(project);
  }
}

void Application::SaveToUserConfig() {
  QJsonObject json;
  QJsonArray projects_arr;
  for (const Project& project: projects) {
    QJsonObject project_obj;
    project_obj["path"] = project.path;
    project_obj["profile"] = project.profile;
    projects_arr.append(project_obj);
  }
  json["projects"] = projects_arr;
  runtime.Schedule<JsonFileProcess>(nullptr, JsonOperation::kWrite,
                                    user_config_path, QJsonDocument(json));
}
