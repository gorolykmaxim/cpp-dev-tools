#include "config.hpp"
#include <QJsonObject>

void UserConfig::LoadFrom(const QJsonDocument& json) {
  QString value = json["open_in_editor_command"].toString();
  cmd_open_file_in_editor = value.contains("{}") ? value : "subl {}";
}

QJsonDocument UserConfig::Save() const {
  QJsonObject json;
  json["open_in_editor_command"] = cmd_open_file_in_editor;
  return QJsonDocument(json);
}
