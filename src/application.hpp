#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "process/process.hpp"
#include "threads.hpp"
#include "config.hpp"

class Application {
public:
  QGuiApplication gui_app;
  QQmlApplicationEngine gui_engine;
  UserConfig user_config;
  ProcessRuntime runtime;
  Threads threads;

  Application(int argc, char** argv);
};
