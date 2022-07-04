#include <functional>
#include <csignal>
#include <algorithm>
#include <iostream>
#include <sys/signal.h>

#include "blockingconcurrentqueue.h"
#include "cdt.h"

static std::vector<PidType> active_process_ids;

std::string OsApi::ReadLineFromStdin() {
  std::string input;
  std::getline(std::cin, input);
  return input;
}

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    setenv(name.c_str(), value.c_str(), true);
}

std::filesystem::path OsApi::GetHome() {
  return GetEnv("HOME");
}

static void StopRunningProcessesOrExit(int signal) {
  if (active_process_ids.empty()) {
    std::signal(signal, SIG_DFL);
    std::raise(signal);
  } else {
    for (PidType id: active_process_ids) {
      kill(id, SIGKILL);
    }
  }
}

void OsApi::Init() {
  std::signal(SIGINT, StopRunningProcessesOrExit);
}

void OsApi::OnProcessStart(BoostProcess& process) {
  active_process_ids.push_back(process.child->id());
}

void OsApi::OnProcessFinish(BoostProcess& process) {
  auto it = std::find(active_process_ids.begin(), active_process_ids.end(),
                      process.child->id());
  if (it != active_process_ids.end()) {
    active_process_ids.erase(it);
  }
}
