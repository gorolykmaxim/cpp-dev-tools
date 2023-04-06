#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThreadPool>
#include <QtConcurrent>
#include <functional>

#include "project_system.h"
#include "task_system.h"
#include "user_command_system.h"
#include "view_system.h"

class Application {
 public:
  static Application& Get();
  Application(int argc, char** argv);
  ~Application();
  int Exec();

  template <typename T>
  void RunIOTask(QObject* requestor, const std::function<T()>& on_io_thread,
                 const std::function<void(T)>& on_ui_thread) {
    auto watcher = new QFutureWatcher<T>();
    QObject::connect(watcher, &QFutureWatcher<T>::finished, requestor,
                     [watcher, on_ui_thread]() {
                       on_ui_thread(watcher->result());
                       watcher->deleteLater();
                     });
    QFuture<T> future = QtConcurrent::run(&io_thread_pool, on_io_thread);
    watcher->setFuture(future);
  }

  void RunIOTask(QObject* requestor, const std::function<void()>& on_io_thread,
                 const std::function<void()>& on_ui_thread);

  int argc_;
  char** argv_;
  ProjectSystem project;
  ViewSystem view;
  UserCommandSystem user_command;
  TaskSystem task;
  QGuiApplication gui_app;
  QQmlApplicationEngine qml_engine;
  QThreadPool io_thread_pool;

  static Application* instance;
};
