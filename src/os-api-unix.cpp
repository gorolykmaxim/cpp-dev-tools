#include <functional>
#include <csignal>
#include <algorithm>
#include <iostream>

#include "blockingconcurrentqueue.h"
#include "cdt.h"
#include "process.hpp"

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
      TinyProcessLib::Process::kill(id);
    }
  }
}

void OsApi::Init() {
  std::signal(SIGINT, StopRunningProcessesOrExit);
}

bool OsApi::StartProcess(
    Process &process,
    moodycamel::BlockingConcurrentQueue<ProcessEvent>& queue,
    entt::entity entity) {
  bool result = StartProcessCommon(process, queue, entity);
  active_process_ids.push_back(process.id);
  return result;
}

void OsApi::FinishProcess(Process& process) {
  auto it = std::find(active_process_ids.begin(), active_process_ids.end(),
                      process.id);
  if (it != active_process_ids.end()) {
    active_process_ids.erase(it);
  }
}
