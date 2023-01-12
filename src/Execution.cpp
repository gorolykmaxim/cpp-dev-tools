#include "Execution.hpp"

Exec* FindExecById(AppData& app, QUuid id) {
  for (Exec& exec: app.execs) {
    if (exec.id == id) {
      return &exec;
    }
  }
  return nullptr;
}

Exec* FindLatestRunningExec(AppData& app) {
  Exec* result = nullptr;
  for (Exec& exec: app.execs) {
    if (!exec.exit_code && exec.proc) {
      result = &exec;
    }
  }
  return result;
}
