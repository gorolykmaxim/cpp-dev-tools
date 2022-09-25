#pragma once

#include "Base.hpp"

class UserConfig {
public:
  UserConfig() = default;
  void LoadFrom(const QJsonDocument& json);
  QJsonDocument Save() const;

  QString cmd_open_file_in_editor;
};
