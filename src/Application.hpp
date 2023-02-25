#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThreadPool>
#include <QtConcurrent>
#include <functional>

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

  int argc_;
  char** argv_;
  QGuiApplication gui_app;
  QQmlApplicationEngine qml_engine;
  QThreadPool io_thread_pool;

  static Application* instance;
};