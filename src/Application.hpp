#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThreadPool>
#include <QtConcurrent>
#include <functional>

#include "Project.hpp"
#include "ProjectContext.hpp"
#include "UserCommandController.hpp"
#include "ViewController.hpp"

class Application {
 public:
  static Application& Get();
  Application(int argc, char** argv);
  int Exec();

  template <typename T>
  void RunIOTask(QObject* requestor, const std::function<T()>& on_io_thread,
                 const std::function<void(T)>& on_ui_thread) {
    auto watcher = QSharedPointer<QFutureWatcher<T>>::create();
    QObject::connect(
        watcher.get(), &QFutureWatcher<T>::finished, requestor,
        [watcher, on_ui_thread]() { on_ui_thread(watcher->result()); });
    QFuture<T> future = QtConcurrent::run(&io_thread_pool, on_io_thread);
    watcher->setFuture(future);
  }

  void RunIOTask(QObject* requestor, const std::function<void()>& on_io_thread,
                 const std::function<void()>& on_ui_thread);

  int argc_;
  char** argv_;
  ProjectContext project_context;
  ViewController view_controller;
  UserCommandController user_command_controller;
  QGuiApplication gui_app;
  QQmlApplicationEngine qml_engine;
  QThreadPool io_thread_pool;

  static Application* instance;
};
