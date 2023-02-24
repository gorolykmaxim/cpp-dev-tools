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
  void RunIOTask(const std::function<T()>& on_io_thread,
                 const std::function<void(T)>& on_ui_thread) {
    (void)QtConcurrent::run(&io_thread_pool, [on_io_thread, on_ui_thread]() {
      T t = on_io_thread();
      QMetaObject::invokeMethod(QGuiApplication::instance(),
                                [t, on_ui_thread]() { on_ui_thread(t); });
    });
  }

  int argc_;
  char** argv_;
  QGuiApplication gui_app;
  QQmlApplicationEngine qml_engine;
  QThreadPool io_thread_pool;

  static Application* instance;
};
