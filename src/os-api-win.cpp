#include <algorithm>
#include <vector>
#include <windows.h>
#include <functional>
#include <mutex>

#include "cdt.h"
#include "process.hpp"

static std::vector<TinyProcessLib::Process::id_type> active_process_ids;
static std::mutex active_process_ids_mtx;

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    SetEnvironmentVariable(name.c_str(), value.c_str());
}

BOOL WINAPI HandleCtrlC(DWORD signal) {
  if (signal != CTRL_C_EVENT) {
    return FALSE;
  }
  std::lock_guard<std::mutex> lock(active_process_ids_mtx);
  for (TinyProcessLib::Process::id_type id: active_process_ids) {
    TinyProcessLib::Process::kill(id);
  }
  return active_process_ids.empty() ? FALSE : TRUE;
}

void OsApi::Init() {
  SetConsoleCtrlHandler(HandleCtrlC, TRUE);
}

void OsApi::StartProcess(
    Process &process,
    const std::function<void (const char *, size_t)> &stdout_cb,
    const std::function<void (const char *, size_t)> &stderr_cb,
    const std::function<void ()> &exit_cb) {
  StartProcessProtected(process, stdout_cb, stderr_cb, exit_cb);
  std::lock_guard<std::mutex> lock(active_process_ids_mtx);
  active_process_ids.push_back(process.handle->get_id());
}

void OsApi::FinishProcess(Process& process) {
  std::lock_guard<std::mutex> lock(active_process_ids_mtx);
  auto it = std::find(active_process_ids.begin(), active_process_ids.end(),
                      process.handle->get_id());
  if (it != active_process_ids.end()) {
    active_process_ids.erase(it);
  }
}
