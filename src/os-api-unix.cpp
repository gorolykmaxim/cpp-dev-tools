#include <functional>
#include <csignal>
#include <algorithm>
#include <iostream>

#include "cdt.h"
#include "process.hpp"

static std::vector<TinyProcessLib::Process::id_type> active_process_ids;

std::string OsApi::ReadLineFromStdin() {
  std::string input;
  std::getline(std::cin, input);
  return input;
}

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    setenv(name.c_str(), value.c_str(), true);
}

static void StopRunningProcessesOrExit(int signal) {
  if (active_process_ids.empty()) {
    std::signal(signal, SIG_DFL);
    std::raise(signal);
  } else {
    for (TinyProcessLib::Process::id_type id: active_process_ids) {
      TinyProcessLib::Process::kill(id);
    }
  }
}

void OsApi::Init() {
  std::signal(SIGINT, StopRunningProcessesOrExit);
}

void OsApi::StartProcess(
    Process &process,
    const std::function<void (const char *, size_t)> &stdout_cb,
    const std::function<void (const char *, size_t)> &stderr_cb,
    const std::function<void ()> &exit_cb) {
  process.handle = std::make_unique<TinyProcessLib::Process>(
      process.shell_command, "", stdout_cb, stderr_cb, exit_cb);
  process.id = process.handle->get_id();
  active_process_ids.push_back(process.handle->get_id());
}

void OsApi::FinishProcess(Process& process) {
  auto it = std::find(active_process_ids.begin(), active_process_ids.end(),
                      process.handle->get_id());
  if (it != active_process_ids.end()) {
    active_process_ids.erase(it);
  }
}
