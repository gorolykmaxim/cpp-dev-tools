#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "ProcessRuntime.hpp"
#include "Threads.hpp"
#include "UserConfig.hpp"

class Application {
public:
  QGuiApplication gui_app;
  QQmlApplicationEngine gui_engine;
  UserConfig user_config;
  ProcessRuntime runtime;
  Threads threads;

  Application(int argc, char** argv);
};