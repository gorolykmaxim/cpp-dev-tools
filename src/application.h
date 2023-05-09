#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThreadPool>
#include <functional>

#include "documentation_system.h"
#include "editor_system.h"
#include "git_system.h"
#include "notification_system.h"
#include "project_system.h"
#include "sqlite_system.h"
#include "task_system.h"
#include "user_command_system.h"
#include "view_system.h"

class Application {
 public:
  static Application& Get();
  Application(int argc, char** argv);
  ~Application();
  int Exec();

  int argc_;
  char** argv_;
  ProjectSystem project;
  ViewSystem view;
  UserCommandSystem user_command;
  TaskSystem task;
  EditorSystem editor;
  NotificationSystem notification;
  DocumentationSystem documentation;
  SqliteSystem sqlite;
  QGuiApplication gui_app;
  QQmlApplicationEngine qml_engine;
  QThreadPool io_thread_pool;

  static Application* instance;
};
