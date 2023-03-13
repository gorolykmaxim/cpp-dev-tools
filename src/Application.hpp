#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThreadPool>
#include <QtConcurrent>
#include <functional>

#include "Project.hpp"
#include "ProjectSystem.hpp"
#include "TaskSystem.hpp"
#include "UserCommandController.hpp"
#include "ViewController.hpp"

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
  ViewController view_controller;
  UserCommandController user_command_controller;
  TaskSystem task;
  QGuiApplication gui_app;
  QQmlApplicationEngine qml_engine;
  QThreadPool io_thread_pool;

  static Application* instance;
};
