#include <windows.h>
#include <functional>

#include "cdt.h"

static std::function<bool()> ctrl_c_handler;

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    SetEnvironmentVariable(name.c_str(), value.c_str());
}

BOOL WINAPI HandleCtrlC(DWORD signal) {
  if (signal != CTRL_C_EVENT) {
    return FALSE;
  }
  return ctrl_c_handler() ? TRUE : FALSE;
}

void OsApi::SetCtrlCHandler(std::function<bool ()> handler) {
  ctrl_c_handler = std::move(handler);
  SetConsoleCtrlHandler(HandleCtrlC, TRUE);
}
