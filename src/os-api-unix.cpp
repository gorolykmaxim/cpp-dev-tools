#include <functional>
#include <csignal>

#include "cdt.h"
#include "process.hpp"

static entt::registry* global_registry;

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    setenv(name.c_str(), value.c_str(), true);
}

static void StopRunningProcessesOrExit(int signal) {
  if (global_registry->view<Process>().empty()) {
    std::signal(signal, SIG_DFL);
    std::raise(signal);
  } else {
    for (auto [_, proc]: global_registry->view<Process>().each()) {
      if (proc.state == ProcessState::kRunning) {
        TinyProcessLib::Process::kill(proc.handle->get_id());
      }
    }
  }
}

void OsApi::SetUpCtrlCHandler(entt::registry &registry) {
  global_registry = &registry;
  std::signal(SIGINT, StopRunningProcessesOrExit);
}

void OsApi::StartProcess(
    Process &process,
    const std::function<void (const char *, size_t)> &stdout_cb,
    const std::function<void (const char *, size_t)> &stderr_cb,
    const std::function<void ()> &exit_cb) {
  StartProcessProtected(process, stdout_cb, stderr_cb, exit_cb);
}
