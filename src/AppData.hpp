#pragma once

#include "Lib.hpp"
#include "ProcessData.hpp"
#include "UIData.hpp"

struct Profile {
  QString& operator[](const QString& key);
  QString operator[](const QString& key) const;
  QString GetName() const;
  QList<QString> GetVariableNames() const;
  bool Contains(const QString& key) const;
private:
  QHash<QString, QString> variables;
};

struct Project {
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

struct Task {
  QString name;
  QString command;
  int flags = 0;
  QVector<QString> pre_tasks;
};

struct AppData {
  QString user_config_path;
  QGuiApplication gui_app;
  QQmlApplicationEngine gui_engine;
  UIActionRouter ui_action_router;
  // view slot name to its view data
  QHash<QString, ViewData> view_data;
  QThreadPool io_thread_pool;
  QVector<QPtr<Process>> processes;
  QStack<ProcessId> free_proc_ids;
  QVector<ProcessId> procs_to_execute;
  QVector<ProcessId> procs_to_finish;
  QVector<Profile> profiles;
  QVector<Task> task_defs;
  QVector<Task> tasks;
  // Last opened project is always first
  QVector<Project> projects;

  AppData(int argc, char** argv);
};
