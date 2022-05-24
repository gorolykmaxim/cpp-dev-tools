#include <algorithm>
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "cdt.h"
#include "execution.h"
#include "common.h"

static Cdt* global_cdt;
static std::string kTask;
static std::string kTaskRepeat;
static std::string kDebug;
static std::string kSelectExecution;

static void TerminateCurrentExecutionOrExit(int signal) {
    if (global_cdt->registry.view<Process>().empty()) {
        global_cdt->os->Signal(signal, SIG_DFL);
        global_cdt->os->RaiseSignal(signal);
    } else {
        for (auto [_, proc]: global_cdt->registry.view<Process>().each()) {
            global_cdt->os->KillProcess(proc);
        }
    }
}

void InitExecution(Cdt& cdt) {
    kTask = DefineUserCommand("t", {"ind", "Execute the task with the specified index"}, cdt);
    kTaskRepeat = DefineUserCommand("tr", {"ind", "Keep executing the task with the specified index until it fails"}, cdt);
    kDebug = DefineUserCommand("d", {"ind", "Execute the task with the specified index with a debugger attached"}, cdt);
    kSelectExecution = DefineUserCommand("exec", {"ind", "Change currently selected execution (gets reset to the most recent one after every new execution)"}, cdt);
    cdt.os->Signal(SIGINT, TerminateCurrentExecutionOrExit);
    global_cdt = &cdt;
}

void ScheduleTask(Cdt& cdt) {
  if (!AcceptUsrCmd(kTask, cdt.last_usr_cmd) &&
      !AcceptUsrCmd(kTaskRepeat, cdt.last_usr_cmd) &&
      !AcceptUsrCmd(kDebug, cdt.last_usr_cmd)) {
    return;
  }
  if (!IsCmdArgInRange(cdt.last_usr_cmd, cdt.tasks)) {
    DisplayListOfTasks(cdt.tasks, cdt);
  } else {
    int task_id = cdt.last_usr_cmd.arg - 1;
    Task& task = cdt.tasks[task_id];
    entt::entity entity = cdt.registry.create();
    Execution& exec = cdt.registry.emplace<Execution>(entity);
    exec.name = task.name;
    exec.shell_command = task.command;
    exec.task_id = task_id;
    exec.repeat_until_fail = kTaskRepeat == cdt.last_usr_cmd.cmd;
    if (kDebug == cdt.last_usr_cmd.cmd) {
      exec.debug = DebugStatus::kRequired;
    }
    cdt.registry.emplace<Output>(entity, OutputMode::kStream);
    cdt.registry.emplace<ToSchedule>(entity);
  }
}

void SchedulePreTasks(Cdt& cdt) {
  std::vector<entt::entity> scheduled;
  for (auto [entity, exec]: cdt.registry.view<Execution, ToSchedule>().each()) {
    for (size_t pre_task_id: cdt.pre_tasks[exec.task_id]) {
      Task& pre_task = cdt.tasks[pre_task_id];
      entt::entity entity_pre_task = cdt.registry.create();
      Execution& exec = cdt.registry.emplace<Execution>(entity_pre_task);
      exec.name = pre_task.name;
      exec.shell_command = pre_task.command;
      exec.task_id = pre_task_id;
      cdt.registry.emplace<Output>(entity_pre_task);
      cdt.execs_to_run.push_back(entity_pre_task);
    }
    scheduled.push_back(entity);
    cdt.execs_to_run.push_back(entity);
  }
  cdt.registry.erase<ToSchedule>(scheduled.begin(), scheduled.end());
}

void ExecuteRestartTask(Cdt& cdt) {
  if (cdt.execs_to_run.empty() || !cdt.registry.view<Process>().empty()) {
    return;
  }
  entt::entity entity = cdt.execs_to_run.front();
  if (cdt.registry.get<Execution>(entity).shell_command != "__restart") {
    return;
  }
  cdt.execs_to_run.pop_front();
  cdt.registry.destroy(entity);
  cdt.os->Out() << kTcMagenta << "Restarting program..." << kTcReset
                << std::endl;
  std::string arg_str = std::to_string(cdt.last_usr_cmd.arg);
  std::string cmd_str = cdt.last_usr_cmd.cmd + arg_str;
  cdt.os->SetEnv(kEnvVarLastCommand, cmd_str);
  int error = cdt.os->Exec({cdt.cdt_executable, cdt.tasks_config_path.c_str(),
                            nullptr});
  if (error) {
    cdt.os->Out() << kTcRed << "Failed to restart: " << std::strerror(error)
                  << kTcReset << std::endl;
  }
}

void StartNextExecution(Cdt& cdt) {
  if (cdt.execs_to_run.empty() || !cdt.registry.view<Process>().empty()) {
    return;
  }
  entt::entity entity = cdt.execs_to_run.front();
  auto [exec, output] = cdt.registry.get<Execution, Output>(entity);
  exec.start_time = cdt.os->TimeNow();
  Process& proc = cdt.registry.emplace<Process>(entity);
  proc.shell_command = exec.shell_command;
  cdt.output = ConsoleOutput{};
  cdt.execs_to_run.pop_front();
  if (cdt.execs_to_run.empty()) {
    cdt.os->Out() << kTcMagenta << "Running \"" << exec.name << "\""
                  << kTcReset << std::endl;
  } else {
    cdt.os->Out() << kTcBlue << "Running \"" << exec.name << "\"..."
                  << kTcReset << std::endl;
  }
}

void DisplayExecutionResult(Cdt& cdt) {
  auto view = cdt.registry.view<Execution, Process, Output>();
  for (auto [_, exec, proc, output]: view.each()) {
    if (proc.state == ProcessState::kRunning) {
      continue;
    }
    int code = cdt.os->GetProcessExitCode(proc);
    if (proc.state == ProcessState::kFailed) {
      cdt.os->Out() << kTcRed << "'" << exec.name << "' failed: return code: "
                    << code << kTcReset << std::endl;
    } else if (cdt.execs_to_run.empty()) {
      if (exec.debug == DebugStatus::kAttached) {
        cdt.os->Out() << kTcMagenta << "Debugger started" << kTcReset
                      << std::endl;
      }
      cdt.os->Out() << kTcMagenta << "'" << exec.name
                    << "' complete: return code: " << code << kTcReset
                    << std::endl;
    }
  }
}

void RestartRepeatingExecutionOnSuccess(Cdt& cdt) {
  std::vector<entt::entity> to_restart;
  auto view = cdt.registry.view<Execution, Process, Output>();
  for (auto [entity, exec, proc, output]: view.each()) {
    if (proc.state != ProcessState::kComplete || !exec.repeat_until_fail) {
      return;
    }
    cdt.execs_to_run.push_back(entity);
    output = Output{output.mode};
    to_restart.push_back(entity);
  }
  cdt.registry.erase<Process>(to_restart.begin(), to_restart.end());
}

void FinishTaskExecution(Cdt& cdt) {
  for (auto [entity, proc, _]: cdt.registry.view<Process, Execution>().each()) {
    if (proc.state == ProcessState::kRunning) {
      continue;
    }
    cdt.exec_history.push_front(entity);
    if (proc.state == ProcessState::kFailed) {
      cdt.registry.destroy(cdt.execs_to_run.begin(), cdt.execs_to_run.end());
      cdt.execs_to_run.clear();
    }
  }
}

void RemoveOldExecutionsFromHistory(Cdt& cdt) {
    static const size_t kMaxHistoryLength = 100;
    std::vector<size_t> to_destroy;
    for (size_t i = cdt.exec_history.size() - 1; cdt.exec_history.size() - to_destroy.size() > kMaxHistoryLength; i--) {
        entt::entity entity = cdt.exec_history[i];
        if (!cdt.registry.get<Execution>(entity).is_pinned && cdt.selected_exec != entity) {
            to_destroy.push_back(i);
        }
    }
    for (size_t i: to_destroy) {
        entt::entity entity = cdt.exec_history[i];
        cdt.exec_history.erase(cdt.exec_history.begin() + i);
        cdt.registry.destroy(entity);
    }
}

void ValidateIfDebuggerAvailable(Cdt& cdt) {
    auto it = std::find(cdt.kUsrCmdNames.begin(), cdt.kUsrCmdNames.end(),
                        cdt.last_usr_cmd.cmd);
    bool is_real_cmd = it != cdt.kUsrCmdNames.end();
    bool is_debug_cmd = cdt.last_usr_cmd.cmd.find('d') != std::string::npos;
    if (cdt.last_usr_cmd.executed || !is_real_cmd || !is_debug_cmd) {
        return;
    }
    if (cdt.debug_cmd.empty()) {
        WarnUserConfigPropNotSpecified(kDebugCommandProperty, cdt);
        cdt.last_usr_cmd.executed = true;
    }
}

void AttachDebuggerToScheduledExecutions(Cdt& cdt) {
  for (entt::entity entity: cdt.execs_to_run) {
    Execution& exec = cdt.registry.get<Execution>(entity);
    if (exec.debug != DebugStatus::kRequired) {
      continue;
    }
    exec.shell_command = FormatTemplate(cdt.debug_cmd, "{shell_cmd}",
                                        exec.shell_command);
    exec.shell_command = FormatTemplate(exec.shell_command, "{current_dir}",
                                        cdt.os->GetCurrentPath().string());
    exec.debug = DebugStatus::kAttached;
  }
}

void ChangeSelectedExecution(Cdt& cdt) {
  static const std::string kCurrentIcon = "->";
  if (!AcceptUsrCmd(kSelectExecution, cdt.last_usr_cmd)) return;
  if (cdt.exec_history.empty()) {
    cdt.os->Out() << kTcGreen << "No task has been executed yet" << kTcReset
                  << std::endl;
  } else if (!IsCmdArgInRange(cdt.last_usr_cmd, cdt.exec_history)) {
    cdt.os->Out() << kTcGreen << "Execution history:" << kTcReset
                  << std::endl;
    entt::entity selected_exec = cdt.selected_exec ?
                                 *cdt.selected_exec :
                                 cdt.exec_history.front();
    for (int i = cdt.exec_history.size() - 1; i >= 0; i--) {
      std::stringstream line;
      entt::entity entity = cdt.exec_history[i];
      Execution& exec = cdt.registry.get<Execution>(entity);
      if (selected_exec == entity) {
        line << kCurrentIcon;
      } else {
        line << std::string(kCurrentIcon.size(), ' ');
      }
      line << ' ' << i + 1 << ' ';
      std::time_t time = std::chrono::system_clock::to_time_t(exec.start_time);
      line << std::put_time(std::localtime(&time), "%T") << ' ';
      line << '"' << exec.name << '"';
      cdt.os->Out() << line.str() << std::endl;
    }
  } else {
    size_t index = cdt.last_usr_cmd.arg - 1;
    if (index == 0) {
      cdt.selected_exec.reset();
      cdt.os->Out() << kTcMagenta << "Selected execution reset"
                    << kTcReset << std::endl;
    } else {
      cdt.selected_exec = cdt.exec_history[index];
      Execution& exec = cdt.registry.get<Execution>(*cdt.selected_exec);
      cdt.os->Out() << kTcMagenta << "Selected execution \"" << exec.name
                    << '"' << kTcReset << std::endl;
    }
    entt::entity entity = cdt.selected_exec ?
                          *cdt.selected_exec :
                          cdt.exec_history.front();
    cdt.output = ConsoleOutput{};
    if (!cdt.registry.all_of<GtestExecution>(entity)) {
      Output& output = cdt.registry.get<Output>(entity);
      cdt.output.lines_displayed = output.lines.size();
      cdt.output.lines = output.lines;
    }
  }
}
