#pragma once

#include "Lib.hpp"
#include "ProcessRuntime.hpp"
#include "Threads.hpp"
#include "UserInterface.hpp"

class Profile {
public:
  QString& operator[](const QString& key);
  QString operator[](const QString& key) const;
  QString GetName() const;
  QList<QString> GetVariableNames() const;
  bool Contains(const QString& key) const;
private:
  QHash<QString, QString> variables;
};

enum TaskFlags {
  kTaskRestart = 1,
  kTaskGtest = 2,
};

class Task {
public:
  QString name;
  QString command;
  int flags = 0;
  QVector<QString> pre_tasks;
};

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
  QVector<Profile> profiles;
  QVector<Task> task_defs;
  QVector<Task> tasks;
  QString current_project_path;
  int current_profile = -1;
  Threads threads;
  UserInterface ui;

  Application(int argc, char** argv);
};
