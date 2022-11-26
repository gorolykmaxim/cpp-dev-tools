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
  QList<QString> pre_tasks;
};

struct AppData {
  QString user_config_path;
  QGuiApplication gui_app;
  QQmlApplicationEngine gui_engine;
  UIActionRouter ui_action_router;
  // view slot name to its view data
  QHash<QString, ViewData> view_data;
  QThreadPool io_thread_pool;
  QList<QPtr<Process>> processes;
  QStack<ProcessId> free_proc_ids;
  QList<ProcessId> procs_to_execute;
  QList<ProcessId> procs_to_finish;
  // Event at the top is the event that should be handled right now
  QQueue<Event> events;
  // Event type to list of processes to wake up
  QHash<QString, QList<ProcessWakeUpCall>> event_listeners;
  QList<Profile> profiles;
  QList<Task> task_defs;
  QList<Task> tasks;
  // Last opened project is always first
  QList<Project> projects;

  AppData(int argc, char** argv);
};