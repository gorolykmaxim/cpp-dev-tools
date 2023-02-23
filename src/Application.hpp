#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThreadPool>

class Application {
 public:
  static Application& Get();
  Application(int argc, char** argv);
  int Exec();

  QGuiApplication gui_app;
  QQmlApplicationEngine qml_engine;
  QThreadPool io_thread_pool;

  static Application* instance;
};
