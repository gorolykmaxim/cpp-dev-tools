#include "Application.hpp"
#include "UI.hpp"

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
      ui_action_router(*this),
      runtime(*this) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  user_config_path = home + "/.cpp-dev-tools.json";
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
  io_thread_pool.setMaxThreadCount(1);
  InitializeUI(*this);
}
