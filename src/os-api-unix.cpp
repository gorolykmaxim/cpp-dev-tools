#include <functional>
#include <csignal>

#include "cdt.h"

static std::function<bool()> ctrl_c_handler;

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    setenv(name.c_str(), value.c_str(), true);
}

static void HandleSignal(int signal) {
  if (!ctrl_c_handler()) {
    std::signal(signal, SIG_DFL);
    raise(signal);
  }
}

void OsApi::SetCtrlCHandler(std::function<bool ()> handler) {
  ctrl_c_handler = std::move(handler);
  std::signal(SIGINT, HandleSignal);
}
