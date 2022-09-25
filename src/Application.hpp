#pragma once

#include "Base.hpp"
#include "ProcessRuntime.hpp"
#include "Threads.hpp"
#include "UserConfig.hpp"
#include "UserInterface.hpp"

class Application {
public:
  QGuiApplication gui_app;
  UserConfig user_config;
  ProcessRuntime runtime;
  Threads threads;
  UserInterface ui;

  Application(int argc, char** argv);
};
