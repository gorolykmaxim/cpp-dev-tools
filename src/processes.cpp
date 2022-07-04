#include "processes.h"
#include "cdt.h"
#include <functional>
#include <ostream>
#include <sstream>

void StartProcesses(Cdt& cdt) {
  for (auto [entity, proc]: cdt.registry.view<Process>().each()) {
    if (proc.state != ProcessState::kScheduled) {
      continue;
    }
    if (!cdt.os->StartProcess(proc, cdt.proc_event_queue, entity)) {
      cdt.os->Out() << "Failed to exec: " << proc.shell_command << std::endl;
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
    proc.state = cdt.os->GetProcessExitCode(proc) == 0 ?
                 ProcessState::kComplete :
                 ProcessState::kFailed;
    cdt.os->FinishProcess(proc);
  } else {
    std::string& line_buffer = event.type == ProcessEventType::kStdout ?
                               output.stdout_line_buffer :
                               output.stderr_line_buffer;
    std::string to_process = line_buffer + event.data;
    std::string::size_type pos = 0;
    while (true) {
      std::string::size_type eol_pos = to_process.find('\n', pos);
      if (eol_pos == std::string::npos) {
        break;
      }
      std::string::size_type next_pos = eol_pos + 1;
      if (eol_pos > 0 && to_process[eol_pos - 1] == '\r') {
        eol_pos--;
      }
      output.lines.push_back(to_process.substr(pos, eol_pos - pos));
      pos = next_pos;
    }
    line_buffer = to_process.substr(pos, to_process.size() - pos);
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
