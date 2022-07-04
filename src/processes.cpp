#include "processes.h"
#include "cdt.h"
#include <functional>
#include <ostream>
#include <sstream>
#include <string>

void StartProcesses(Cdt& cdt) {
  for (auto [entity, proc]: cdt.registry.view<Process>().each()) {
    if (proc.state != ProcessState::kScheduled) {
      continue;
    }
    std::string error;
    if (!cdt.os->StartProcess(entity, cdt.registry, cdt.proc_event_queue,
                              error)) {
      cdt.os->Out() << kTcRed << "Failed to exec: " << proc.shell_command
                    << std::endl << error << kTcReset << std::endl;
    }
    proc.state = ProcessState::kRunning;
  }
}

void HandleProcessEvent(Cdt& cdt) {
  if (cdt.registry.view<Process>().empty()) return;
  ProcessEvent event;
  cdt.proc_event_queue.wait_dequeue(event);
  auto [proc, output] = cdt.registry.get<Process, Output>(event.process);
  if (event.type == ProcessEventType::kExit) {
    cdt.os->FinishProcess(event.process, cdt.registry);
    proc.state = proc.exit_code == 0 ?
                 ProcessState::kComplete :
                 ProcessState::kFailed;
  } else {
    if (event.data.back() == '\r') {
      event.data.erase(event.data.size() - 1, 1);
    }
    output.lines.push_back(event.data);
  }
}

void FinishProcessExecution(Cdt& cdt) {
  std::vector<entt::entity> to_erase;
  std::vector<entt::entity> to_destroy;
  for (auto [entity, proc]: cdt.registry.view<Process>().each()) {
    if (proc.state == ProcessState::kRunning) {
      continue;
    }
    if (proc.destroy_entity_on_finish) {
      to_destroy.push_back(entity);
    } else {
      to_erase.push_back(entity);
    }
  }
  cdt.registry.erase<Process>(to_erase.begin(), to_erase.end());
  cdt.registry.destroy(to_destroy.begin(), to_destroy.end());
}
