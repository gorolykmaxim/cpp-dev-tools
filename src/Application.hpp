#pragma once

#include "Lib.hpp"
#include "ProcessRuntime.hpp"
#include "Threads.hpp"
#include "UserInterface.hpp"

class UserConfig {
public:
  UserConfig() = default;
  void LoadFrom(const QJsonDocument& json);
  QJsonDocument Save() const;

  QString cmd_open_file_in_editor;
};

class Application {
public:
  QGuiApplication gui_app;
  UserConfig user_config;
  ProcessRuntime runtime;
  QVector<QHash<QString, QString>> profiles;
  Threads threads;
  UserInterface ui;

  Application(int argc, char** argv);
};
