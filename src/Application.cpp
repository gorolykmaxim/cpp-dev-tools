#include "Application.hpp"

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

void UserConfig::LoadFrom(const QJsonDocument& json) {
  QString value = json["open_in_editor_command"].toString();
  cmd_open_file_in_editor = value.contains("{}") ? value : "subl {}";
}

QJsonDocument UserConfig::Save() const {
  QJsonObject json;
  json["open_in_editor_command"] = cmd_open_file_in_editor;
  return QJsonDocument(json);
}

Application::Application(int argc, char** argv)
    : gui_app(argc, argv),
      runtime(*this),
      threads(gui_app),
      ui() {
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
}
