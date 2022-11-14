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

class Project {
public:
  Project(const QString& path);
  bool operator==(const Project& project) const;
  bool operator!=(const Project& project) const;

  QString path;
  int profile;
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

class Application {
public:
  QString user_config_path;
  QGuiApplication gui_app;
  ProcessRuntime runtime;
  QVector<Profile> profiles;
  QVector<Task> task_defs;
  QVector<Task> tasks;
  // Last opened project is always first
  QVector<Project> projects;
  Threads threads;
  UserInterface ui;

  Application(int argc, char** argv);
  void LoadFrom(const QJsonDocument& json);
  void SaveToUserConfig();
};
