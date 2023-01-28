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
  QString GetPathRelativeToHome() const;
  QString GetFolderName() const;
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
  QString src_name;
  QString src_cmd;
  QList<QString> src_pre_tasks;
  QString name;
  QString cmd;
  int flags = 0;
  QList<QString> pre_tasks;
};

struct Exec {
  QUuid id;
  QUuid primary_exec_id;
  QString task_name;
  QString cmd;
  QDateTime start_time;
  QPtr<QProcess> proc;
  std::optional<int> exit_code = {};
  QString output;
};

QDebug operator<<(QDebug debug, const Exec& exec);

struct UserCmd {
  QString group;
  QString name;
  QString shortcut;
  std::function<void()> callback;

  QString GetFormattedShortcut() const;
};

struct AppData {
  QString user_config_path;
  QString current_project_path;
  QGuiApplication gui_app;
  QQmlApplicationEngine gui_engine;
  QSqlDatabase db;
  UIActionRouter ui_action_router;
  // View slot name to its view data
  QHash<QString, ViewData> view_data;
  QThreadPool io_thread_pool;
  QList<QPtr<Process>> processes;
  QStack<ProcessId> free_proc_ids;
  QSet<ProcessId> procs_to_execute;
  QSet<ProcessId> procs_to_finish;
  // Name to process id
  QHash<QString, ProcessId> named_processes;
  // Event at the top is the event that should be handled right now
  QQueue<Event> events;
  // Event type to list of processes to wake up
  QHash<QString, QList<ProcessWakeUpCall>> event_listeners;
  QList<Profile> profiles;
  QList<Task> tasks;
  QList<Exec> execs;
  // Last opened project is always first
  QList<Project> projects;
  // Event type to user command to execute
  QHash<QString, UserCmd> user_cmds;
  QList<QString> user_command_events_ordered;

  AppData(int argc, char** argv);
};
