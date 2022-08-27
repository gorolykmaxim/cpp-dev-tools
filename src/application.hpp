#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "process.hpp"
#include "threads.hpp"

class Application {
public:
  QGuiApplication gui_app;
  QQmlApplicationEngine gui_engine;
  ProcessRuntime runtime;
  Threads threads;

  Application(int argc, char** argv);
};
