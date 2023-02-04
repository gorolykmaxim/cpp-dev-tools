#pragma once

#include "Lib.hpp"
#include "ProcessData.hpp"
#include "UIData.hpp"

struct Project {
  QUuid id;
  QString path;
  bool is_opened = false;
  QDateTime last_open_time;

  QString GetPathRelativeToHome() const;
  QString GetFolderName() const;
};

struct Exec {
  QUuid id;
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
  QPtr<Project> current_project;
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
  QList<Exec> execs;
  // Event type to user command to execute
  QHash<QString, UserCmd> user_cmds;
  QList<QString> user_command_events_ordered;

  AppData(int argc, char** argv);
};
