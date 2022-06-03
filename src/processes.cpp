#include "processes.h"
#include <sstream>

static std::function<void(const char*,size_t)> WriteTo(
    moodycamel::BlockingConcurrentQueue<ProcessEvent>& queue,
    entt::entity process, ProcessEventType event_type) {
  return [&queue, process, event_type] (const char* data, size_t size) {
    ProcessEvent event;
    event.process = process;
    event.type = event_type;
    event.data = std::string(data, size);
    queue.enqueue(event);
  };
}

static std::function<void()> HandleExit(
    moodycamel::BlockingConcurrentQueue<ProcessEvent>& queue,
    entt::entity process) {
  return [&queue, process] () {
    ProcessEvent event;
    event.process = process;
    event.type = ProcessEventType::kExit;
    queue.enqueue(event);
  };
}

void StartProcesses(Cdt& cdt) {
  for (auto [entity, proc]: cdt.registry.view<Process>().each()) {
    if (proc.state != ProcessState::kScheduled) {
      continue;
    }
    cdt.os->StartProcess(
        proc,
        WriteTo(cdt.proc_event_queue, entity, ProcessEventType::kStdout),
        WriteTo(cdt.proc_event_queue, entity, ProcessEventType::kStderr),
        HandleExit(cdt.proc_event_queue, entity));
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
    std::stringstream tmp_buffer;
    for (char c: to_process) {
      if (c == '\n') {
        output.lines.push_back(tmp_buffer.str());
        tmp_buffer = std::stringstream();
      } else {
        tmp_buffer << c;
      }
    }
    line_buffer = tmp_buffer.str();
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
