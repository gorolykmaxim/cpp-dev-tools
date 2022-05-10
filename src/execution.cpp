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
    if (global_cdt->registry.view<Running>().empty()) {
        global_cdt->os->Signal(signal, SIG_DFL);
        global_cdt->os->RaiseSignal(signal);
    } else {
        for (auto [_, proc]: global_cdt->registry.view<Process, Running>().each()) {
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
    if (!AcceptUsrCmd(kTask, cdt.last_usr_cmd) && !AcceptUsrCmd(kTaskRepeat, cdt.last_usr_cmd) && !AcceptUsrCmd(kDebug, cdt.last_usr_cmd)) return;
    if (!IsCmdArgInRange(cdt.last_usr_cmd, cdt.tasks)) {
        DisplayListOfTasks(cdt.tasks, cdt);
    } else {
        int task_id = cdt.last_usr_cmd.arg - 1;
        Task& task = cdt.tasks[task_id];
        entt::entity entity = cdt.registry.create();
        Execution& exec = cdt.registry.emplace<Execution>(entity);
        exec.name = task.name;
        exec.task_id = task_id;
        exec.repeat_until_fail = kTaskRepeat == cdt.last_usr_cmd.cmd;
        Process& proc = cdt.registry.emplace<Process>(entity);
        proc.shell_command = task.command;
        proc.stream_output = true;
        if (kDebug == cdt.last_usr_cmd.cmd) {
            proc.debug = DebugStatus::kRequired;
        }
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
            exec.task_id = pre_task_id;
            Process& proc = cdt.registry.emplace<Process>(entity_pre_task);
            proc.shell_command = pre_task.command;
            cdt.execs_to_run.push_back(entity_pre_task);
        }
        scheduled.push_back(entity);
        cdt.execs_to_run.push_back(entity);
    }
    cdt.registry.erase<ToSchedule>(scheduled.begin(), scheduled.end());
}

void ExecuteRestartTask(Cdt& cdt) {
    if (cdt.execs_to_run.empty() || !cdt.registry.view<Running>().empty()) return;
    entt::entity entity = cdt.execs_to_run.front();
    Process& proc = cdt.registry.get<Process>(entity);
    if (proc.shell_command != "__restart") {
        return;
    }
    cdt.execs_to_run.pop_front();
    cdt.registry.destroy(entity);
    cdt.os->Out() << kTcMagenta << "Restarting program..." << kTcReset << std::endl;
    std::string cmd_str = cdt.last_usr_cmd.cmd + std::to_string(cdt.last_usr_cmd.arg);
    cdt.os->SetEnv(kEnvVarLastCommand, cmd_str);
    int error = cdt.os->Exec({cdt.cdt_executable, cdt.tasks_config_path.c_str(), nullptr});
    if (error) {
        cdt.os->Out() << kTcRed << "Failed to restart: " << std::strerror(error) << kTcReset << std::endl;
    }
}

static std::function<void(const char*,size_t)> WriteTo(moodycamel::BlockingConcurrentQueue<ProcessEvent>& queue, entt::entity process, ProcessEventType event_type) {
    return [&queue, process, event_type] (const char* data, size_t size) {
        ProcessEvent event;
        event.process = process;
        event.type = event_type;
        event.data = std::string(data, size);
        queue.enqueue(event);
    };
}

static std::function<void()> HandleExit(moodycamel::BlockingConcurrentQueue<ProcessEvent>& queue, entt::entity process) {
    return [&queue, process] () {
        ProcessEvent event;
        event.process = process;
        event.type = ProcessEventType::kExit;
        queue.enqueue(event);
    };
}

void StartNextExecution(Cdt& cdt) {
    if (cdt.execs_to_run.empty() || !cdt.registry.view<Running>().empty()) return;
    entt::entity entity = cdt.execs_to_run.front();
    auto [exec, proc] = cdt.registry.get<Execution, Process>(entity);
    exec.start_time = cdt.os->TimeNow();
    cdt.os->StartProcess(
        proc,
        WriteTo(cdt.proc_event_queue, entity, ProcessEventType::kStdout),
        WriteTo(cdt.proc_event_queue, entity, ProcessEventType::kStderr),
        HandleExit(cdt.proc_event_queue, entity)
    );
    cdt.registry.emplace<ExecutionOutput>(entity);
    cdt.registry.emplace<TextBuffer>(entity);
    cdt.registry.emplace<Running>(entity);
    cdt.execs_to_run.pop_front();
    if (proc.stream_output) {
        cdt.os->Out() << kTcMagenta << "Running \"" << exec.name << "\"" << kTcReset << std::endl;
    } else {
        cdt.os->Out() << kTcBlue << "Running \"" << exec.name << "\"..." << kTcReset << std::endl;
    }
}

void HandleProcessEvent(Cdt& cdt) {
    if (cdt.registry.view<Running>().empty()) return;
    ProcessEvent event;
    cdt.proc_event_queue.wait_dequeue(event);
    auto [proc, exec, tb] = cdt.registry.get<Process, Execution, TextBuffer>(event.process);
    if (event.type == ProcessEventType::kExit) {
        exec.state = cdt.os->GetProcessExitCode(proc) == 0 ? ExecutionState::kComplete : ExecutionState::kFailed;
    } else {
        std::vector<std::string>& buffer = tb.buffers[kBufferProcess];
        std::string& line_buffer = event.type == ProcessEventType::kStdout ? proc.stdout_line_buffer : proc.stderr_line_buffer;
        std::string to_process = line_buffer + event.data;
        std::stringstream tmp_buffer;
        for (char c: to_process) {
            if (c == '\n') {
                buffer.push_back(tmp_buffer.str());
                tmp_buffer = std::stringstream();
            } else {
                tmp_buffer << c;
            }
        }
        line_buffer = tmp_buffer.str();
    }
}

void DisplayExecutionResult(Cdt& cdt) {
    for (auto [_, exec, proc]: cdt.registry.view<Execution, Process, Running>().each()) {
        if (exec.state == ExecutionState::kRunning) {
            continue;
        }
        int code = cdt.os->GetProcessExitCode(proc);
        if (exec.state == ExecutionState::kFailed) {
            cdt.os->Out() << kTcRed << "'" << exec.name << "' failed: return code: " << code << kTcReset << std::endl;
        } else if (proc.stream_output) {
            if (proc.debug == DebugStatus::kAttached) {
                cdt.os->Out() << kTcMagenta << "Debugger started" << kTcReset << std::endl;
            }
            cdt.os->Out() << kTcMagenta << "'" << exec.name << "' complete: return code: " << code << kTcReset << std::endl;
        }
    }
}

void RestartRepeatingExecutionOnSuccess(Cdt& cdt) {
    std::vector<entt::entity> to_restart;
    for (auto [entity, exec, proc]: cdt.registry.view<Execution, Process, Running>().each()) {
        if (exec.state != ExecutionState::kComplete || !exec.repeat_until_fail) {
            return;
        }
        exec.state = ExecutionState::kRunning;
        proc.handle = nullptr;
        cdt.registry.erase<TextBuffer, ExecutionOutput>(entity);
        cdt.execs_to_run.push_back(entity);
        to_restart.push_back(entity);
    }
    cdt.registry.erase<Running>(to_restart.begin(), to_restart.end());
}

void FinishTaskExecution(Cdt& cdt) {
    std::vector<entt::entity> to_finish;
    for (auto [entity, exec]: cdt.registry.view<Execution, Running>().each()) {
        if (exec.state == ExecutionState::kRunning) {
            continue;
        }
        to_finish.push_back(entity);
        cdt.exec_history.push_front(entity);
        if (exec.state == ExecutionState::kFailed) {
            cdt.registry.destroy(cdt.execs_to_run.begin(), cdt.execs_to_run.end());
            cdt.execs_to_run.clear();
        }
    }
    cdt.registry.erase<Running>(to_finish.begin(), to_finish.end());
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
    bool is_real_cmd = std::find(cdt.kUsrCmdNames.begin(), cdt.kUsrCmdNames.end(), cdt.last_usr_cmd.cmd) != cdt.kUsrCmdNames.end();
    bool is_debug_cmd = cdt.last_usr_cmd.cmd.find('d') != std::string::npos;
    if (cdt.last_usr_cmd.executed || !is_real_cmd || !is_debug_cmd) {
        return;
    }
    std::vector<std::string> mandatory_props_not_specified;
    if (cdt.debug_cmd.str.empty()) mandatory_props_not_specified.push_back(kDebugCommandProperty);
    if (cdt.execute_in_new_terminal_tab_cmd.str.empty()) mandatory_props_not_specified.push_back(kExecuteInNewTerminalTabCommandProperty);
    if (!mandatory_props_not_specified.empty()) {
        for (std::string& prop: mandatory_props_not_specified) {
            WarnUserConfigPropNotSpecified(prop, cdt);
        }
        cdt.last_usr_cmd.executed = true;
    }
}

void AttachDebuggerToScheduledExecutions(Cdt& cdt) {
    for (entt::entity entity: cdt.execs_to_run) {
        Process& proc = cdt.registry.get<Process>(entity);
        if (proc.debug != DebugStatus::kRequired) {
            continue;
        }
        std::string debug_cmd = FormatTemplate(cdt.debug_cmd, proc.shell_command);
        std::string cmds_to_exec = "cd " + cdt.os->GetCurrentPath().string() + " && " + debug_cmd;
        proc.shell_command = FormatTemplate(cdt.execute_in_new_terminal_tab_cmd, cmds_to_exec);
        proc.debug = DebugStatus::kAttached;
    }
}

void ChangeSelectedExecution(Cdt& cdt) {
    static const std::string kCurrentIcon = "->";
    if (!AcceptUsrCmd(kSelectExecution, cdt.last_usr_cmd)) return;
    if (cdt.exec_history.empty()) {
        cdt.os->Out() << kTcGreen << "No task has been executed yet" << kTcReset << std::endl;
    } else if (!IsCmdArgInRange(cdt.last_usr_cmd, cdt.exec_history)) {
        cdt.os->Out() << kTcGreen << "Execution history:" << kTcReset << std::endl;
        entt::entity selected_exec = cdt.selected_exec ? *cdt.selected_exec : cdt.exec_history.front();
        for (int i = cdt.exec_history.size() - 1; i >= 0; i--) {
            std::stringstream line;
            entt::entity entity = cdt.exec_history[i];
            Execution& exec = cdt.registry.get<Execution>(entity);
            line << (selected_exec == entity ? kCurrentIcon : std::string(kCurrentIcon.size(), ' ')) << ' ';
            line << i + 1 << ' ';
            std::time_t time = std::chrono::system_clock::to_time_t(exec.start_time);
            line << std::put_time(std::localtime(&time), "%T") << ' ';
            line << '"' << exec.name << '"';
            cdt.os->Out() << line.str() << std::endl;
        }
    } else {
        size_t index = cdt.last_usr_cmd.arg - 1;
        if (index == 0) {
            cdt.selected_exec.reset();
            cdt.os->Out() << kTcMagenta << "Selected execution reset" << kTcReset << std::endl;
        } else {
            cdt.selected_exec = cdt.exec_history[index];
            cdt.os->Out() << kTcMagenta << "Selected execution \"" << cdt.registry.get<Execution>(*cdt.selected_exec).name << '"' << kTcReset << std::endl;
        }
    }
}
