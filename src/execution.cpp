#include <sstream>

#include "execution.h"
#include "common.h"

static Cdt* global_cdt;
static std::string kTask;
static std::string kTaskRepeat;
static std::string kDebug;
static std::string kSelectExecution;

static void TerminateCurrentExecutionOrExit(int signal) {
    if (global_cdt->processes.empty()) {
        global_cdt->os->Signal(signal, SIG_DFL);
        global_cdt->os->RaiseSignal(signal);
    } else {
        for (const auto& [e, _]: global_cdt->processes) {
            global_cdt->os->KillProcess(e, global_cdt->processes);
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
        const auto id = cdt.last_usr_cmd.arg - 1;
        for (const auto pre_task_id: cdt.pre_tasks[id]) {
            const auto pre_task_exec = CreateEntity(cdt);
            cdt.task_ids[pre_task_exec] = pre_task_id;
            cdt.execs_to_run_in_order.push_back(pre_task_exec);
        }
        const auto task_exec = CreateEntity(cdt);
        cdt.task_ids[task_exec] = id;
        cdt.stream_output.insert(task_exec);
        if (kTaskRepeat == cdt.last_usr_cmd.cmd) {
            cdt.repeat_until_fail.insert(task_exec);
        }
        if (kDebug == cdt.last_usr_cmd.cmd) {
            cdt.debug.insert(task_exec);
        }
        cdt.execs_to_run_in_order.push_back(task_exec);
    }
}

void ExecuteRestartTask(Cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    Entity exec = cdt.execs_to_run_in_order.front();
    if (cdt.tasks[cdt.task_ids[exec]].command != "__restart") return;
    cdt.execs_to_run_in_order.pop_front();
    DestroyEntity(exec, cdt);
    cdt.os->Out() << kTcMagenta << "Restarting program..." << kTcReset << std::endl;
    const auto cmd_str = cdt.last_usr_cmd.cmd + std::to_string(cdt.last_usr_cmd.arg);
    cdt.os->SetEnv(kEnvVarLastCommand, cmd_str);
    int error = cdt.os->Exec({cdt.cdt_executable, cdt.tasks_config_path.c_str(), nullptr});
    if (error) {
        cdt.os->Out() << kTcRed << "Failed to restart: " << std::strerror(error) << kTcReset << std::endl;
    }
}

void ScheduleTaskExecutions(Cdt& cdt) {
    for (const auto exec: cdt.execs_to_run_in_order) {
        if (!Find(exec, cdt.execs)) {
            const auto& task = cdt.tasks[cdt.task_ids[exec]];
            cdt.execs[exec] = Execution{task.name, task.command};
        }
    }
}

static std::function<void(const char*,size_t)> WriteTo(moodycamel::BlockingConcurrentQueue<ExecutionEvent>& queue, Entity exec, ExecutionEventType event_type) {
    return [&queue, exec, event_type] (const char* data, size_t size) {
        ExecutionEvent event;
        event.exec = exec;
        event.type = event_type;
        event.data = std::string(data, size);
        queue.enqueue(event);
    };
}

static std::function<void()> HandleExit(moodycamel::BlockingConcurrentQueue<ExecutionEvent>& queue, Entity exec) {
    return [&queue, exec] () {
        ExecutionEvent event;
        event.exec = exec;
        event.type = ExecutionEventType::kExit;
        queue.enqueue(event);
    };
}

void StartNextExecution(Cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    Entity exec_entity = cdt.execs_to_run_in_order.front();
    Execution& exec = cdt.execs[exec_entity];
    exec.start_time = cdt.os->TimeNow();
    cdt.os->StartProcess(
        exec_entity,
        exec.shell_command,
        WriteTo(cdt.exec_event_queue, exec_entity, ExecutionEventType::kStdout),
        WriteTo(cdt.exec_event_queue, exec_entity, ExecutionEventType::kStderr),
        HandleExit(cdt.exec_event_queue, exec_entity),
        cdt.processes
    );
    cdt.exec_outputs[exec_entity] = ExecutionOutput{};
    if (Find(exec_entity, cdt.stream_output)) {
        cdt.os->Out() << kTcMagenta << "Running \"" << exec.name << "\"" << kTcReset << std::endl;
    } else {
        cdt.os->Out() << kTcBlue << "Running \"" << exec.name << "\"..." << kTcReset << std::endl;
    }
}

void ProcessExecutionEvent(Cdt& cdt) {
    if (cdt.processes.empty()) return;
    ExecutionEvent event;
    cdt.exec_event_queue.wait_dequeue(event);
    auto& exec = cdt.execs[event.exec];
    auto& buffer = cdt.text_buffers[event.exec][TextBufferType::kProcess];
    if (event.type == ExecutionEventType::kExit) {
        exec.state = cdt.os->GetProcessExitCode(event.exec, cdt.processes) == 0 ? ExecutionState::kComplete : ExecutionState::kFailed;
    } else {
        auto& line_buffer = event.type == ExecutionEventType::kStdout ? exec.stdout_line_buffer : exec.stderr_line_buffer;
        auto to_process = line_buffer + event.data;
        std::stringstream tmp_buffer;
        for (const auto c: to_process) {
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
    for (auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state != ExecutionState::kRunning) {
            const auto code = cdt.os->GetProcessExitCode(proc.first, cdt.processes);
            if (exec.state == ExecutionState::kFailed) {
                cdt.os->Out() << kTcRed << "'" << exec.name << "' failed: return code: " << code << kTcReset << std::endl;
            } else if (Find(proc.first, cdt.stream_output)) {
                cdt.os->Out() << kTcMagenta << "'" << exec.name << "' complete: return code: " << code << kTcReset << std::endl;
            }
        }
    }
}

void RestartRepeatingExecutionOnSuccess(Cdt& cdt) {
    std::vector<Entity> to_destroy;
    for (const auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state == ExecutionState::kComplete && Find(proc.first, cdt.repeat_until_fail)) {
            cdt.execs[proc.first] = Execution{exec.name, exec.shell_command};
            cdt.text_buffers[proc.first][TextBufferType::kOutput].clear();
            to_destroy.push_back(proc.first);
        }
    }
    DestroyComponents(to_destroy, cdt.processes);
}

void FinishTaskExecution(Cdt& cdt) {
    std::vector<Entity> to_destroy;
    std::vector<Entity> processes_to_remove;
    for (auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state != ExecutionState::kRunning) {
            cdt.execs_to_run_in_order.pop_front();
            cdt.exec_history.push_front(proc.first);
            if (exec.state == ExecutionState::kFailed) {
                to_destroy.insert(to_destroy.begin(), cdt.execs_to_run_in_order.begin(), cdt.execs_to_run_in_order.end());
                cdt.execs_to_run_in_order.clear();
            }
            processes_to_remove.push_back(proc.first);
        }
    }
    for (const auto e: to_destroy) {
        DestroyEntity(e, cdt);
    }
    DestroyComponents(processes_to_remove, cdt.processes);
}

void RemoveOldExecutionsFromHistory(Cdt& cdt) {
    static const size_t kMaxHistoryLength = 100;
    std::vector<size_t> to_destroy;
    for (size_t i = cdt.exec_history.size() - 1; cdt.exec_history.size() - to_destroy.size() > kMaxHistoryLength; i--) {
        Entity entity = cdt.exec_history[i];
        if (!cdt.execs[entity].is_pinned && cdt.selected_exec != entity) {
            to_destroy.push_back(i);
        }
    }
    for (size_t i: to_destroy) {
        Entity entity = cdt.exec_history[i];
        cdt.exec_history.erase(cdt.exec_history.begin() + i);
        DestroyEntity(entity, cdt);
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
        for (const auto& prop: mandatory_props_not_specified) {
            WarnUserConfigPropNotSpecified(prop, cdt);
        }
        cdt.last_usr_cmd.executed = true;
    }
}

void StartNextExecutionWithDebugger(Cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    Entity exec_entity = cdt.execs_to_run_in_order.front();
    if (!Find(exec_entity, cdt.debug)) return;
    cdt.execs_to_run_in_order.pop_front();
    cdt.exec_history.push_front(exec_entity);
    Execution& exec = cdt.execs[exec_entity];
    exec.start_time = cdt.os->TimeNow();
    exec.state = ExecutionState::kComplete;
    std::string debug_cmd = FormatTemplate(cdt.debug_cmd, exec.shell_command);
    std::string cmds_to_exec = "cd " + cdt.os->GetCurrentPath().string() + " && " + debug_cmd;
    std::string shell_cmd = FormatTemplate(cdt.execute_in_new_terminal_tab_cmd, cmds_to_exec);
    cdt.os->Out() << kTcMagenta << "Starting debugger for \"" << exec.name << "\"..." << kTcReset << std::endl;
    cdt.os->ExecProcess(shell_cmd);
}

void ChangeSelectedExecution(Cdt& cdt) {
    static const std::string kCurrentIcon = "->";
    if (!AcceptUsrCmd(kSelectExecution, cdt.last_usr_cmd)) return;
    if (cdt.exec_history.empty()) {
        cdt.os->Out() << kTcGreen << "No task has been executed yet" << kTcReset << std::endl;
    } else if (!IsCmdArgInRange(cdt.last_usr_cmd, cdt.exec_history)) {
        cdt.os->Out() << kTcGreen << "Execution history:" << kTcReset << std::endl;
        Entity selected_exec = cdt.selected_exec ? *cdt.selected_exec : cdt.exec_history.front();
        for (int i = cdt.exec_history.size() - 1; i >= 0; i--) {
            std::stringstream line;
            Entity entity = cdt.exec_history[i];
            Execution& exec = cdt.execs[entity];
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
            cdt.os->Out() << kTcMagenta << "Selected execution \"" << cdt.execs[*cdt.selected_exec].name << '"' << kTcReset << std::endl;
        }
    }
}
