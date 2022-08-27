#pragma once

#include "process.h"
#include "threads.h"

class Application {
public:
  ProcessRuntime runtime;
  Threads threads;

  Application(QObject& ui_context): runtime(*this), threads(ui_context) {}
};
