#pragma once

#include "process.hpp"
#include "threads.hpp"

class Application {
public:
  ProcessRuntime runtime;
  Threads threads;

  Application(QObject& ui_context): runtime(*this), threads(ui_context) {}
};
