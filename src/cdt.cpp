#include "cdt.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <ios>
#include <iostream>
#include <fstream>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <csignal>
#include <string>
#include <unistd.h>
#include <regex>
#include <unordered_map>
#include "json.hpp"
#include "process.hpp"

const auto kTcRed = "\033[31m";
const auto kTcGreen = "\033[32m";
const auto kTcBlue = "\033[34m";
const auto kTcMagenta = "\033[35m";
const auto kTcReset = "\033[0m";

const auto kOpenInEditorCommandProperty = "open_in_editor_command";
const auto kExecuteInNewTerminalTabCommandProperty = "execute_in_new_terminal_tab_command";
const auto kDebugCommandProperty = "debug_command";
const auto kEnvVarLastCommand = "LAST_COMMAND";
const std::string kTemplateArgPlaceholder = "{}";
const std::string kGtestTask = "__gtest";
const std::string kGtestFilterArg = "--gtest_filter";

template <typename T>
T PushBackAndReturn(std::vector<T>& vec, T&& t) {
    vec.push_back(t);
    return t;
}

Cdt* global_cdt;
std::vector<UserCommandDefinition> kUsrCmdDefs;
const auto kTask = PushBackAndReturn(kUsrCmdDefs, {"t", "ind", "Execute the task with the specified index"});
const auto kTaskRepeat = PushBackAndReturn(kUsrCmdDefs, {"tr", "ind", "Keep executing the task with the specified index until it fails"});
const auto kDebug = PushBackAndReturn(kUsrCmdDefs, {"d", "ind", "Execute the task with the specified index with a debugger attached"});
const auto kOpen = PushBackAndReturn(kUsrCmdDefs, {"o", "ind", "Open the file link with the specified index in your code editor"});
const auto kSearch = PushBackAndReturn(kUsrCmdDefs, {"s", "", "Search through output of the last executed task with the specified regular expression"});
const auto kGtest = PushBackAndReturn(kUsrCmdDefs, {"g", "ind", "Display output of the specified google test"});
const auto kGtestSearch = PushBackAndReturn(kUsrCmdDefs, {"gs", "ind", "Search through output of the specified google test with the specified regular expression"});
const auto kGtestRerun = PushBackAndReturn(kUsrCmdDefs, {"gt", "ind", "Re-run the google test with the specified index"});
const auto kGtestRerunRepeat = PushBackAndReturn(kUsrCmdDefs, {"gtr", "ind", "Keep re-running the google test with the specified index until it fails"});
const auto kGtestDebug = PushBackAndReturn(kUsrCmdDefs, {"gd", "ind", "Re-run the google test with the specified index with debugger attached"});
const auto kGtestFilter = PushBackAndReturn(kUsrCmdDefs, {"gf", "ind", "Run google tests of the task with the specified index with a specified " + kGtestFilterArg});
const auto kHelp = PushBackAndReturn(kUsrCmdDefs, {"h", "", "Display list of user commands"});

std::istream& OsApi::In() {
    return std::cin;
}

std::ostream& OsApi::Out() {
    return std::cout;
}

std::ostream& OsApi::Err() {
    return std::cerr;
}

std::string OsApi::GetEnv(const std::string &name) {
    const char* value = getenv(name.c_str());
    return value ? value : "";
}

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    setenv(name.c_str(), value.c_str(), true);
}

void OsApi::SetCurrentPath(const std::filesystem::path &path) {
    std::filesystem::current_path(path);
}

std::filesystem::path OsApi::GetCurrentPath() {
    return std::filesystem::current_path();
}

bool OsApi::ReadFile(const std::filesystem::path &path, std::string &data) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    file >> std::noskipws;
    data = std::string(std::istream_iterator<char>(file), std::istream_iterator<char>());
    return true;
}

void OsApi::WriteFile(const std::filesystem::path &path, const std::string &data) {
    std::ofstream file(path);
    file << data;
}

bool OsApi::FileExists(const std::filesystem::path &path) {
    return std::filesystem::exists(path);
}

void OsApi::Signal(int signal, void (*handler)(int)) {
    std::signal(signal, handler);
}

void OsApi::RaiseSignal(int signal) {
    std::raise(signal);
}

int OsApi::Exec(const std::vector<const char *> &args) {
    execvp(args[0], const_cast<char* const*>(args.data()));
    return errno;
}

void OsApi::KillProcess(Entity e, std::unordered_map<Entity, std::unique_ptr<TinyProcessLib::Process>> &processes) {
    TinyProcessLib::Process::kill(processes[e]->get_id());
}

void OsApi::ExecProcess(const std::string &shell_cmd) {
    TinyProcessLib::Process(shell_cmd).get_exit_status();
}

void OsApi::StartProcess(Entity e,
                           const std::string &shell_cmd,
                           const std::function<void (const char *, size_t)> &stdout_cb,
                           const std::function<void (const char *, size_t)> &stderr_cb,
                           const std::function<void ()> &exit_cb,
                           std::unordered_map<Entity, std::unique_ptr<TinyProcessLib::Process>> &processes) {
    processes[e] = std::make_unique<TinyProcessLib::Process>(shell_cmd, "", stdout_cb, stderr_cb, exit_cb);
}

int OsApi::GetProcessExitCode(Entity e, std::unordered_map<Entity, std::unique_ptr<TinyProcessLib::Process>> &processes) {
    return processes[e]->get_exit_status();
}

static Entity CreateEntity(Cdt& cdt) {
    return cdt.entity_seed++;
}

static void DestroyEntity(Entity e, Cdt& cdt) {
    cdt.execs_to_run_in_order.erase(std::remove(cdt.execs_to_run_in_order.begin(), cdt.execs_to_run_in_order.end(), e), cdt.execs_to_run_in_order.end());
    cdt.task_ids.erase(e);
    cdt.processes.erase(e);
    cdt.execs.erase(e);
    cdt.exec_outputs.erase(e);
    cdt.gtest_execs.erase(e);
    cdt.repeat_until_fail.erase(e);
    cdt.stream_output.erase(e);
    for (int t = static_cast<int>(TextBufferType::kProcess); t <= static_cast<int>(TextBufferType::kOutput); t++) {
        cdt.text_buffers[static_cast<TextBufferType>(t)].erase(e);
    }
}

template <typename T>
static T* Find(Entity e, std::unordered_map<Entity, T>& components) {
    auto res = components.find(e);
    return res != components.end() ? &res->second : nullptr;
}

static bool Find(Entity e, const std::unordered_set<Entity>& components) {
    return components.count(e) > 0;
}


template <typename T>
static void MoveComponent(Entity source, Entity target, std::unordered_map<Entity, T>& components) {
    if (Find(source, components)) {
        components[target] = std::move(components[source]);
        components.erase(source);
    }
}

template <typename T>
static void DestroyComponents(const std::unordered_set<Entity>& es, std::unordered_map<Entity, T>& components) {
    for (auto e: es) {
        components.erase(e);
    }
}

static void MoveTextBuffer(Entity e, TextBufferType from, TextBufferType to, std::unordered_map<TextBufferType, std::unordered_map<Entity, std::vector<std::string>>>& text_buffers) {
    auto& f = text_buffers[from][e];
    auto& t = text_buffers[to][e];
    t.reserve(t.size() + f.size());
    t.insert(t.end(), f.begin(), f.end());
    f.clear();
}

static void WarnUserConfigPropNotSpecified(const std::string& property, Cdt& cdt) {
    cdt.os->Out() << kTcRed << '\'' << property << "' is not specified in " << cdt.user_config_path << kTcReset << std::endl;
}

static std::string FormatTemplate(const TemplateString& templ, const std::string& str) {
    std::string res = templ.str;
    res.replace(templ.arg_pos, kTemplateArgPlaceholder.size(), str);
    return res;
}

static bool AcceptUsrCmd(const UserCommandDefinition& def, UserCommand& cmd) {
    if (!cmd.executed && def.cmd == cmd.cmd) {
        cmd.executed = true;
        return true;
    } else {
        return false;
    }
}

template <typename T>
static bool ReadProperty(const nlohmann::json& json, const std::string& name, T& value,
                         bool is_optional, const std::string& err_prefix, const std::string& err_suffix,
                         std::vector<std::string>& errors, std::function<bool()> validate_result = [] {return true;}) {
    auto it = json.find(name);
    const auto err_msg = err_prefix + "'" + name + "': " + err_suffix;
    if (it == json.end()) {
        if (!is_optional) {
            errors.emplace_back(err_msg);
        }
        return false;
    }
    try {
        value = it->get<T>();
        if (!validate_result()) {
            errors.emplace_back(err_msg);
            return false;
        } else {
            return true;
        }
    } catch (std::exception& e) {
        errors.emplace_back(err_msg);
        return false;
    }
}

static void PreTaskNamesToIndexes(const std::vector<std::string>& pre_task_names, const std::vector<nlohmann::json>& tasks_json,
                                  std::vector<size_t>& pre_tasks, const std::string& err_prefix, std::vector<std::string>& errors) {
    for (const auto& pre_task_name: pre_task_names) {
        auto pre_task_id = -1;
        for (auto i = 0; i < tasks_json.size(); i++) {
            const auto& another_task = tasks_json[i];
            const auto& another_task_name = another_task.find("name");
            if (another_task_name != another_task.end() && *another_task_name == pre_task_name) {
                pre_task_id = i;
                break;
            }
        }
        if (pre_task_id >= 0) {
            pre_tasks.push_back(pre_task_id);
        } else {
            errors.emplace_back(err_prefix + "references task '" + pre_task_name + "' that does not exist");
        }
    }
}

static bool ReadArgv(int argc, const char** argv, Cdt& cdt) {
    if (argc < 2) {
        cdt.config_errors.emplace_back("usage: cpp-dev-tools tasks.json");
        return false;
    }
    cdt.tasks_config_path = std::filesystem::absolute(argv[1]);
    const auto& config_dir_path = cdt.tasks_config_path.parent_path();
    cdt.os->SetCurrentPath(config_dir_path);
    return true;
}

static bool ParseJsonFile(const std::filesystem::path& config_path, bool should_exist, nlohmann::json& config_json,
                            std::vector<std::string>& errors, Cdt& cdt) {
    std::string config_data;
    if (!cdt.os->ReadFile(config_path, config_data)) {
        if (should_exist) {
            errors.push_back(config_path.string() + " does not exist");
        }
        return false;
    }
    try {
        config_json = nlohmann::json::parse(config_data, nullptr, true, true);
        return true;
    } catch (std::exception& e) {
        errors.emplace_back("Failed to parse " + config_path.string() + ": " + e.what());
        return false;
    }
}

static void AppendConfigErrors(const std::filesystem::path& config_path, const std::vector<std::string>& from,
                                 std::vector<std::string>& to) {
    if (!from.empty()) {
        to.emplace_back(config_path.string() + " is invalid:");
        to.insert(to.end(), from.begin(), from.end());
    }
}

static void InitExampleUserConfig(Cdt& cdt) {
    cdt.user_config_path = std::filesystem::path(cdt.os->GetEnv("HOME")) / ".cpp-dev-tools.json";
    if (!cdt.os->FileExists(cdt.user_config_path)) {
        std::stringstream example;
        example << "{" << std::endl;
        example << "  // Open file links from the output in Sublime Text:" << std::endl;
        example << "  //\"" << kOpenInEditorCommandProperty << "\": \"subl {}\"" << std::endl;
        example << "  // Open file links from the output in VSCode:" << std::endl;
        example << "  //\"" << kOpenInEditorCommandProperty << "\": \"code {}\"" << std::endl;
        example << "  // Execute in a new terminal tab on MacOS:" << std::endl;
        example << "  // \"" << kExecuteInNewTerminalTabCommandProperty << "\": \"osascript -e 'tell application \\\"Terminal\\\" to do script \\\"{}\\\"'\"" << std::endl;
        example << "  // Execute in a new terminal tab on Windows:" << std::endl;
        example << "  // \"" << kExecuteInNewTerminalTabCommandProperty << "\": \"/c/Program\\ Files/Git/git-bash -c '{}'\"" << std::endl;
        example << "  // Debug tasks via lldb:" << std::endl;
        example << "  //\"" << kDebugCommandProperty << "\": \"lldb -- {}\"" << std::endl;
        example << "  // Debug tasks via gdb:" << std::endl;
        example << "  //\"" << kDebugCommandProperty << "\": \"gdb --args {}\"" << std::endl;
        example << "}" << std::endl;
        cdt.os->WriteFile(cdt.user_config_path, example.str());
    }
}

static std::function<bool()> ParseTemplateString(TemplateString& temp_str) {
    return [&temp_str] () {
        const auto pos = temp_str.str.find(kTemplateArgPlaceholder);
        if (pos == std::string::npos) {
            return false;
        } else {
            temp_str.arg_pos = pos;
            return true;
        }
    };
}

static void ReadUserConfig(Cdt& cdt) {
    static const std::string kErrSuffix = "must be a string in format, examples of which you can find in the config";
    nlohmann::json config_json;
    if (!ParseJsonFile(cdt.user_config_path, false, config_json, cdt.config_errors, cdt)) {
        return;
    }
    std::vector<std::string> config_errors;
    ReadProperty(config_json, kOpenInEditorCommandProperty, cdt.open_in_editor_cmd.str, true, "", kErrSuffix, config_errors, ParseTemplateString(cdt.open_in_editor_cmd));
    ReadProperty(config_json, kDebugCommandProperty, cdt.debug_cmd.str, true, "", kErrSuffix, config_errors, ParseTemplateString(cdt.debug_cmd));
    ReadProperty(config_json, kExecuteInNewTerminalTabCommandProperty, cdt.execute_in_new_terminal_tab_cmd.str, true, "", kErrSuffix, config_errors, ParseTemplateString(cdt.execute_in_new_terminal_tab_cmd));
    AppendConfigErrors(cdt.user_config_path, config_errors, cdt.config_errors);
}

static void ReadTasksConfig(Cdt& cdt) {
    nlohmann::json config_json;
    std::vector<std::string> config_errors;
    if (!ParseJsonFile(cdt.tasks_config_path, true, config_json, cdt.config_errors, cdt)) {
        return;
    }
    std::vector<nlohmann::json> tasks_json;
    if (!ReadProperty(config_json, "cdt_tasks", tasks_json, false, "", "must be an array of task objects", config_errors)) {
        AppendConfigErrors(cdt.tasks_config_path, config_errors, cdt.config_errors);
        return;
    }
    std::vector<std::vector<size_t>> direct_pre_tasks(tasks_json.size());
    cdt.pre_tasks = std::vector<std::vector<size_t>>(tasks_json.size());
    // Initialize tasks with their direct "pre_tasks" dependencies
    for (auto i = 0; i < tasks_json.size(); i++) {
        Task new_task;
        const auto& task_json = tasks_json[i];
        const auto err_prefix = "task #" + std::to_string(i + 1) + ": ";
        ReadProperty(task_json, "name", new_task.name, false, err_prefix, "must be a string", config_errors);
        ReadProperty(task_json, "command", new_task.command, false, err_prefix, "must be a string", config_errors);
        std::vector<std::string> pre_task_names;
        if (ReadProperty(task_json, "pre_tasks", pre_task_names, true, err_prefix, "must be an array of other task names", config_errors)) {
            PreTaskNamesToIndexes(pre_task_names, tasks_json, direct_pre_tasks[i], err_prefix, config_errors);
        }
        cdt.tasks.push_back(new_task);
    }
    // Transform the "pre_tasks" dependency graph of each task into a flat vector of effective pre_tasks.
    for (auto i = 0; i < cdt.tasks.size(); i++) {
        const auto& primary_task_name = cdt.tasks[i].name;
        auto& pre_tasks = cdt.pre_tasks[i];
        std::stack<size_t> to_visit;
        std::vector<size_t> task_call_stack;
        to_visit.push(i);
        task_call_stack.push_back(i);
        while (!to_visit.empty()) {
            const auto task_id = to_visit.top();
            const auto& pre_pre_tasks = direct_pre_tasks[task_id];
            // We are visiting each task with non-empty "pre_tasks" twice, so when we get to a task the second time - the
            // task should already be on top of the task_call_stack. If that's the case - don't push the task to stack
            // second time.
            if (task_call_stack.back() != task_id) {
                task_call_stack.push_back(task_id);
            }
            auto all_children_visited = true;
            for (const auto& child: pre_pre_tasks) {
                if (std::find(pre_tasks.begin(), pre_tasks.end(), child) == pre_tasks.end()) {
                    all_children_visited = false;
                    break;
                }
            }
            if (all_children_visited) {
                to_visit.pop();
                task_call_stack.pop_back();
                // Primary task is also in to_visit. Don't add it to pre_tasks.
                if (!to_visit.empty()) {
                    pre_tasks.push_back(task_id);
                }
            } else {
                // Check for circular dependencies
                if (std::count(task_call_stack.begin(), task_call_stack.end(), task_id) > 1) {
                    std::stringstream err;
                    err << "task '" << primary_task_name << "' has a circular dependency in it's 'pre_tasks':\n";
                    for (auto j = 0; j < task_call_stack.size(); j++) {
                        err << cdt.tasks[task_call_stack[j]].name;
                        if (j + 1 < task_call_stack.size()) {
                            err << " -> ";
                        }
                    }
                    config_errors.emplace_back(err.str());
                    break;
                }
                for (auto it = pre_pre_tasks.rbegin(); it != pre_pre_tasks.rend(); it++) {
                    to_visit.push(*it);
                }
            }
        }
    }
    AppendConfigErrors(cdt.tasks_config_path, config_errors, cdt.config_errors);
}

static bool PrintErrors(const Cdt& cdt) {
    if (cdt.config_errors.empty()) return false;
    cdt.os->Err() << kTcRed;
    for (const auto& e: cdt.config_errors) {
        cdt.os->Err() << e << std::endl;
    }
    cdt.os->Err() << kTcReset;
    return true;
}

static UserCommand ParseUserCommand(const std::string& str) {
    std::stringstream chars;
    std::stringstream digits;
    for (auto i = 0; i < str.size(); i++) {
        auto c = str[i];
        if (std::isspace(c)) {
            continue;
        }
        if (std::isdigit(c)) {
            digits << c;
        } else {
            chars << c;
        }
    }
    UserCommand cmd;
    cmd.cmd = chars.str();
    cmd.arg = std::atoi(digits.str().c_str());
    return cmd;
}

template<typename T>
static bool IsCmdArgInRange(const UserCommand& cmd, const std::vector<T>& range) {
    return cmd.arg > 0 && cmd.arg <= range.size();
}

static std::string ReadInputFromStdin(const std::string& prefix, Cdt& cdt) {
    cdt.os->Out() << kTcGreen << prefix << kTcReset;
    std::string input;
    std::getline(cdt.os->In(), input);
    return input;
}

static void ReadUserCommandFromStdin(Cdt& cdt) {
    if (!WillWaitForInput(cdt)) return;
    const auto input = ReadInputFromStdin("(cdt) ", cdt);
    if (input.empty()) {
        cdt.last_usr_cmd.executed = false;
    } else {
        cdt.last_usr_cmd = ParseUserCommand(input);
    }
}

static void ReadUserCommandFromEnv(Cdt& cdt) {
    const auto str = cdt.os->GetEnv(kEnvVarLastCommand);
    if (!str.empty()) {
        cdt.last_usr_cmd = ParseUserCommand(str);
    }
}

static void DisplayListOfTasks(const std::vector<Task>& tasks, Cdt& cdt) {
    cdt.os->Out() << kTcGreen << "Tasks:" << kTcReset << std::endl;
    for (auto i = 0; i < tasks.size(); i++) {
        cdt.os->Out() << i + 1 << " \"" << tasks[i].name << "\"" << std::endl;
    }
}

static void ScheduleTask(Cdt& cdt) {
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
        if (kTaskRepeat.cmd == cdt.last_usr_cmd.cmd) {
            cdt.repeat_until_fail.insert(task_exec);
        }
        if (kDebug.cmd == cdt.last_usr_cmd.cmd) {
            cdt.debug.insert(task_exec);
        }
        cdt.execs_to_run_in_order.push_back(task_exec);
    }
}

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

static void ExecuteRestartTask(Cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    const auto exec = cdt.execs_to_run_in_order.front();
    if (cdt.tasks[cdt.task_ids[exec]].command != "__restart") return;
    DestroyEntity(exec, cdt);
    cdt.os->Out() << kTcMagenta << "Restarting program..." << kTcReset << std::endl;
    const auto cmd_str = cdt.last_usr_cmd.cmd + std::to_string(cdt.last_usr_cmd.arg);
    cdt.os->SetEnv(kEnvVarLastCommand, cmd_str);
    int error = cdt.os->Exec({cdt.cdt_executable, cdt.tasks_config_path.c_str(), nullptr});
    if (error) {
        cdt.os->Out() << kTcRed << "Failed to restart: " << std::strerror(error) << kTcReset << std::endl;
    }
}

static std::string GtestTaskToShellCommand(const Task& task, const std::optional<std::string>& gtest_filter = {}) {
    auto binary = task.command.substr(kGtestTask.size() + 1);
    if (gtest_filter) {
        binary += " " + kGtestFilterArg + "='" + *gtest_filter + "'";
    }
    return binary;
}

static void ScheduleGtestExecutions(Cdt& cdt) {
    for (const auto exec: cdt.execs_to_run_in_order) {
        const auto& task = cdt.tasks[cdt.task_ids[exec]];
        if (task.command.find(kGtestTask) == 0) {
            if (!Find(exec, cdt.execs)) {
                cdt.execs[exec] = Execution{task.name, GtestTaskToShellCommand(task)};
            }
            if (!Find(exec, cdt.gtest_execs)) {
                cdt.gtest_execs[exec] = GtestExecution{};
            }
        }
    }
}

static void ScheduleTaskExecutions(Cdt& cdt) {
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

static void StartNextExecution(Cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    const auto exec_entity = cdt.execs_to_run_in_order.front();
    const auto& exec = cdt.execs[exec_entity];
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

static void ProcessExecutionEvent(Cdt& cdt) {
    if (cdt.processes.empty()) return;
    ExecutionEvent event;
    cdt.exec_event_queue.wait_dequeue(event);
    auto& exec = cdt.execs[event.exec];
    auto& buffer = cdt.text_buffers[TextBufferType::kProcess][event.exec];
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

static void CompleteCurrentGtest(GtestExecution& gtest_exec, const std::string& line_content, bool& test_completed) {
    gtest_exec.tests[*gtest_exec.current_test].duration = line_content.substr(line_content.rfind('('));
    gtest_exec.current_test.reset();
    test_completed = true;
}

static void ParseGtestOutput(Cdt& cdt) {
    static const auto kTestCountIndex = std::string("Running ").size();
    for (const auto& proc: cdt.processes) {
        auto& proc_buffer = cdt.text_buffers[TextBufferType::kProcess][proc.first];
        auto& gtest_buffer = cdt.text_buffers[TextBufferType::kGtest][proc.first];
        auto& out_buffer = cdt.text_buffers[TextBufferType::kOutput][proc.first];
        auto& exec_output = cdt.exec_outputs[proc.first];
        const auto gtest_exec = Find(proc.first, cdt.gtest_execs);
        const auto stream_output = Find(proc.first, cdt.stream_output);
        if (!gtest_exec) {
            continue;
        }
        if (gtest_exec->state == GtestExecutionState::kRunning || gtest_exec->state == GtestExecutionState::kParsing) {
            for (const auto& line: proc_buffer) {
                auto line_content_index = 0;
                auto filler_char = '\0';
                std::stringstream word_stream;
                for (auto i = 0; i < line.size(); i++) {
                    if (i == 0) {
                        if (line[i] == '[') {
                            continue;
                        } else {
                            break;
                        }
                    }
                    if (i == 1) {
                        filler_char = line[i];
                        continue;
                    }
                    if (line[i] == ']') {
                        line_content_index = i + 2;
                        break;
                    }
                    if (line[i] != filler_char) {
                        word_stream << line[i];
                    }
                }
                const auto found_word = word_stream.str();
                const auto line_content = line.substr(line_content_index);
                bool test_completed = false;
                /*
                The order of conditions here is important:
                We might be executing google tests, that execute their own google tests as a part of their
                routine. We must differentiate between the output of tests launched by us and any other output
                that might look like google test output.
                As a first priority - we handle the output as a part of output of the current test. When getting
                "OK" and "FAILED" we also check that those two actually belong to OUR current test.
                Only in case we are not running a test it is safe to process other cases, since they are guaranteed
                to belong to our tests and not some other child tests.
                */
                if (found_word == "OK" && line_content.find(gtest_exec->tests.back().name) == 0) {
                    CompleteCurrentGtest(*gtest_exec, line_content, test_completed);
                } else if (found_word == "FAILED" && line_content.find(gtest_exec->tests.back().name) == 0) {
                    gtest_exec->failed_test_ids.push_back(*gtest_exec->current_test);
                    CompleteCurrentGtest(*gtest_exec, line_content, test_completed);
                } else if (gtest_exec->current_test) {
                    gtest_exec->tests[*gtest_exec->current_test].buffer_end++;
                    gtest_buffer.push_back(line);
                    if (stream_output && gtest_exec->rerun_of_single_test) {
                        out_buffer.push_back(line);
                    }
                } else if (found_word == "RUN") {
                    gtest_exec->current_test = gtest_exec->tests.size();
                    GtestTest test{line_content};
                    test.buffer_start = gtest_buffer.size();
                    test.buffer_end = test.buffer_start;
                    gtest_exec->tests.push_back(test);
                } else if (filler_char == '=') {
                    if (gtest_exec->state == GtestExecutionState::kRunning) {
                        const auto count_end_index = line_content.find(' ', kTestCountIndex);
                        const auto count_str = line_content.substr(kTestCountIndex, count_end_index - kTestCountIndex);
                        gtest_exec->test_count = std::stoi(count_str);
                        gtest_exec->tests.reserve(gtest_exec->test_count);
                        gtest_exec->state = GtestExecutionState::kParsing;
                    } else {
                        gtest_exec->total_duration = line_content.substr(line_content.rfind('('));
                        gtest_exec->state = GtestExecutionState::kParsed;
                        break;
                    }
                }
                if (stream_output && !gtest_exec->rerun_of_single_test && test_completed) {
                    cdt.os->Out() << std::flush << "\rTests completed: " << gtest_exec->tests.size() << " of " << gtest_exec->test_count;
                }
            }
        }
        proc_buffer.clear();
    }
}

static void PrintGtestList(const std::vector<size_t>& test_ids, const std::vector<GtestTest>& tests, Cdt& cdt) {
    for (auto i = 0; i < test_ids.size(); i++) {
        const auto id = test_ids[i];
        const auto& test = tests[id];
        cdt.os->Out() << i + 1 << " \"" << test.name << "\" " << test.duration << std::endl;
    }
}

static void PrintFailedGtestList(const GtestExecution& exec, Cdt& cdt) {
    cdt.os->Out() << kTcRed << "Failed tests:" << kTcReset << std::endl;
    PrintGtestList(exec.failed_test_ids, exec.tests, cdt);
    const int failed_percent = exec.failed_test_ids.size() / (float)exec.tests.size() * 100;
    cdt.os->Out() << kTcRed << "Tests failed: " << exec.failed_test_ids.size() << " of " << exec.tests.size() << " (" << failed_percent << "%) " << exec.total_duration << kTcReset << std::endl;
}

static void PrintGtestOutput(ExecutionOutput& out, const std::vector<std::string>& gtest_buffer, std::vector<std::string>& out_buffer, const GtestTest& test, const std::string& color) {
    out = ExecutionOutput{};
    out_buffer.clear();
    out_buffer.reserve(test.buffer_end - test.buffer_start + 1);
    out_buffer.emplace_back(color + "\"" + test.name + "\" output:" + kTcReset);
    out_buffer.insert(out_buffer.end(), gtest_buffer.begin() + test.buffer_start, gtest_buffer.begin() + test.buffer_end);
}

static GtestTest* FindGtestByCmdArg(const UserCommand& cmd, GtestExecution* exec, Cdt& cdt) {
    if (!exec || exec->test_count == 0) {
        cdt.os->Out() << kTcGreen << "No google tests have been executed yet." << kTcReset << std::endl;
    } else if (exec->failed_test_ids.empty()) {
        if (!IsCmdArgInRange(cmd, exec->tests)) {
            cdt.os->Out() << kTcGreen << "Last executed tests " << exec->total_duration << ":" << kTcReset << std::endl;
            std::vector<size_t> ids(exec->tests.size());
            std::iota(ids.begin(), ids.end(), 0);
            PrintGtestList(ids, exec->tests, cdt);
        } else {
            return &exec->tests[cmd.arg - 1];
        }
    } else {
        if (!IsCmdArgInRange(cmd, exec->failed_test_ids)) {
            PrintFailedGtestList(*exec, cdt);
        } else {
            return &exec->tests[exec->failed_test_ids[cmd.arg - 1]];
        }
    }
    return nullptr;
}

static void DisplayGtestOutput(Cdt& cdt) {
    if (!AcceptUsrCmd(kGtest, cdt.last_usr_cmd)) return;
    const auto last_gtest_exec = Find(cdt.last_entity, cdt.gtest_execs);
    const auto last_exec_output = Find(cdt.last_entity, cdt.exec_outputs);
    const auto& gtest_buffer = cdt.text_buffers[TextBufferType::kGtest][cdt.last_entity];
    auto& out_buffer = cdt.text_buffers[TextBufferType::kOutput][cdt.last_entity];
    const auto test = FindGtestByCmdArg(cdt.last_usr_cmd, last_gtest_exec, cdt);
    if (last_gtest_exec && last_exec_output && test) {
        PrintGtestOutput(*last_exec_output, gtest_buffer, out_buffer, *test, last_gtest_exec->failed_test_ids.empty() ? kTcGreen : kTcRed);
    }
}

static void SearchThroughGtestOutput(Cdt& cdt) {
    if (!AcceptUsrCmd(kGtestSearch, cdt.last_usr_cmd)) return;
    const auto last_gtest_exec = Find(cdt.last_entity, cdt.gtest_execs);
    const auto test = FindGtestByCmdArg(cdt.last_usr_cmd, last_gtest_exec, cdt);
    if (test) {
        cdt.text_buffer_searchs[cdt.last_entity] = TextBufferSearch{TextBufferType::kGtest, test->buffer_start, test->buffer_end};
    }
}

static void RerunGtest(Cdt& cdt) {
    if (!AcceptUsrCmd(kGtestRerun, cdt.last_usr_cmd) && !AcceptUsrCmd(kGtestRerunRepeat, cdt.last_usr_cmd) && !AcceptUsrCmd(kGtestDebug, cdt.last_usr_cmd)) return;
    const auto last_gtest_exec = Find(cdt.last_entity, cdt.gtest_execs);
    const auto gtest_task_id = Find(cdt.last_entity, cdt.task_ids);
    const auto test = FindGtestByCmdArg(cdt.last_usr_cmd, last_gtest_exec, cdt);
    if (last_gtest_exec && gtest_task_id && test) {
        for (const auto& pre_task_id: cdt.pre_tasks[*gtest_task_id]) {
            const auto pre_task_exec = CreateEntity(cdt);
            cdt.task_ids[pre_task_exec] = pre_task_id;
            cdt.execs_to_run_in_order.push_back(pre_task_exec);
        }
        const auto exec = CreateEntity(cdt);
        cdt.task_ids[exec] = *gtest_task_id;
        cdt.execs[exec] = Execution{test->name, GtestTaskToShellCommand(cdt.tasks[*gtest_task_id], test->name)};
        cdt.gtest_execs[exec] = GtestExecution{true};
        cdt.stream_output.insert(exec);
        if (kGtestRerunRepeat.cmd == cdt.last_usr_cmd.cmd) {
            cdt.repeat_until_fail.insert(exec);
        }
        if (kGtestDebug.cmd == cdt.last_usr_cmd.cmd) {
            cdt.debug.insert(exec);
        }
        cdt.execs_to_run_in_order.push_back(exec);
    }
}

static void ScheduleGtestTaskWithFilter(Cdt& cdt) {
    if (!AcceptUsrCmd(kGtestFilter, cdt.last_usr_cmd)) return;
    if (!IsCmdArgInRange(cdt.last_usr_cmd, cdt.tasks)) {
        DisplayListOfTasks(cdt.tasks, cdt);
    } else {
        const auto filter = ReadInputFromStdin(kGtestFilterArg + "=", cdt);
        const auto task_id = cdt.last_usr_cmd.arg - 1;
        for (const auto pre_task_id: cdt.pre_tasks[task_id]) {
            const auto pre_task_exec = CreateEntity(cdt);
            cdt.task_ids[pre_task_exec] = pre_task_id;
            cdt.execs_to_run_in_order.push_back(pre_task_exec);
        }
        const auto exec = CreateEntity(cdt);
        cdt.task_ids[exec] = task_id;
        cdt.execs[exec] = Execution{filter, GtestTaskToShellCommand(cdt.tasks[task_id], filter)};
        cdt.stream_output.insert(exec);
        cdt.execs_to_run_in_order.push_back(exec);
    }
}

static void DisplayGtestExecutionResult(Cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        const auto task_id = cdt.task_ids[proc.first];
        auto& exec = cdt.execs[proc.first];
        auto& exec_output = cdt.exec_outputs[proc.first];
        const auto gtest_exec = Find(proc.first, cdt.gtest_execs);
        if (gtest_exec && !gtest_exec->rerun_of_single_test && exec.state != ExecutionState::kRunning && gtest_exec->state != GtestExecutionState::kFinished) {
            cdt.os->Out() << "\33[2K\r"; // Current line has test execution progress displayed. We will start displaying our output on top of it.
            if (gtest_exec->state == GtestExecutionState::kRunning) {
                exec.state = ExecutionState::kFailed;
                cdt.os->Out() << kTcRed << "'" << GtestTaskToShellCommand(cdt.tasks[task_id]) << "' is not a google test executable" << kTcReset << std::endl;
            } else if (gtest_exec->state == GtestExecutionState::kParsing) {
                exec.state = ExecutionState::kFailed;
                cdt.os->Out() << kTcRed << "Tests have finished prematurely" << kTcReset << std::endl;
                // Tests might have crashed in the middle of some test. If so - consider test failed.
                // This is not always true however: tests might have crashed in between two test cases.
                if (gtest_exec->current_test) {
                    gtest_exec->failed_test_ids.push_back(*gtest_exec->current_test);
                    gtest_exec->current_test.reset();
                }
            } else if (gtest_exec->failed_test_ids.empty() && Find(proc.first, cdt.stream_output)) {
                cdt.os->Out() << kTcGreen << "Successfully executed " << gtest_exec->tests.size() << " tests "  << gtest_exec->total_duration << kTcReset << std::endl;
            }
            if (!gtest_exec->failed_test_ids.empty()) {
                PrintFailedGtestList(*gtest_exec, cdt);
                if (gtest_exec->failed_test_ids.size() == 1) {
                    const auto& test = gtest_exec->tests[gtest_exec->failed_test_ids[0]];
                    const auto& gtest_buffer = cdt.text_buffers[TextBufferType::kGtest][proc.first];
                    auto& out_buffer = cdt.text_buffers[TextBufferType::kOutput][proc.first];
                    PrintGtestOutput(exec_output, gtest_buffer, out_buffer, test, kTcRed);
                }
            }
            gtest_exec->state = GtestExecutionState::kFinished;
        }
    }
}

static void RestartRepeatingGtestOnSuccess(Cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        const auto gtest_exec = Find(proc.first, cdt.gtest_execs);
        if (gtest_exec && cdt.execs[proc.first].state == ExecutionState::kComplete && Find(proc.first, cdt.repeat_until_fail)) {
            cdt.gtest_execs[proc.first] = GtestExecution{gtest_exec->rerun_of_single_test};
            cdt.text_buffers[TextBufferType::kGtest][proc.first].clear();
        }
    }
}

static void FinishGtestExecution(Cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        const auto gtest_exec = Find(proc.first, cdt.gtest_execs);
        if (gtest_exec && cdt.execs[proc.first].state != ExecutionState::kRunning && !gtest_exec->rerun_of_single_test) {
            MoveComponent(proc.first, cdt.last_entity, cdt.task_ids);
            MoveComponent(proc.first, cdt.last_entity, cdt.text_buffers[TextBufferType::kGtest]);
            MoveComponent(proc.first, cdt.last_entity, cdt.gtest_execs);
        }
    }
}

static void StreamExecutionOutput(Cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        if (Find(proc.first, cdt.stream_output)) {
            MoveTextBuffer(proc.first, TextBufferType::kProcess, TextBufferType::kOutput, cdt.text_buffers);
        }
    }
}

static void FindAndHighlightFileLinks(Cdt& cdt) {
    static const std::regex kFileLinkRegex("(\\/[^:]+):([0-9]+):?([0-9]+)?");
    if (cdt.open_in_editor_cmd.str.empty()) return;
    for (auto& out: cdt.exec_outputs) {
        auto& buffer = cdt.text_buffers[TextBufferType::kOutput][out.first];
        for (auto i = out.second.lines_processed; i < buffer.size(); i++) {
            auto& line = buffer[i];
            std::stringstream highlighted_line;
            auto last_link_end_pos = 0;
            for (auto it = std::sregex_iterator(line.begin(), line.end(), kFileLinkRegex); it != std::sregex_iterator(); it++) {
                std::stringstream link_stream;
                link_stream << (*it)[1] << ':' << (*it)[2];
                if (it->size() > 3 && (*it)[3].matched) {
                    link_stream << ':' << (*it)[3];
                }
                const auto link = link_stream.str();
                out.second.file_links.push_back(link);
                highlighted_line << line.substr(last_link_end_pos, it->position() - last_link_end_pos) << kTcMagenta << '[' << kOpen.cmd << out.second.file_links.size() << "] " << link << kTcReset;
                last_link_end_pos = it->position() + it->length();
            }
            highlighted_line << line.substr(last_link_end_pos, line.size() - last_link_end_pos);
            line = highlighted_line.str();
        }
    }
}

static void PrintExecutionOutput(Cdt& cdt) {
    for (auto& out: cdt.exec_outputs) {
        auto& buffer = cdt.text_buffers[TextBufferType::kOutput][out.first];
        for (auto i = out.second.lines_processed; i < buffer.size(); i++) {
            cdt.os->Out() << buffer[i] << std::endl;
        }
        out.second.lines_processed = buffer.size();
    }
}

static void StreamExecutionOutputOnFailure(Cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        if (cdt.execs[proc.first].state == ExecutionState::kFailed && !Find(proc.first, cdt.stream_output)) {
            MoveTextBuffer(proc.first, TextBufferType::kProcess, TextBufferType::kOutput, cdt.text_buffers);
        }
    }
}

static void DisplayExecutionResult(Cdt& cdt) {
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

static void RestartRepeatingExecutionOnSuccess(Cdt& cdt) {
    std::unordered_set<Entity> to_destroy;
    for (const auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state == ExecutionState::kComplete && Find(proc.first, cdt.repeat_until_fail)) {
            cdt.execs[proc.first] = Execution{exec.name, exec.shell_command};
            cdt.text_buffers[TextBufferType::kOutput][proc.first].clear();
            to_destroy.insert(proc.first);
        }
    }
    DestroyComponents(to_destroy, cdt.processes);
}

static void FinishTaskExecution(Cdt& cdt) {
    std::unordered_set<Entity> to_destroy;
    for (auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state != ExecutionState::kRunning) {
            if (exec.state == ExecutionState::kFailed) {
                to_destroy.insert(cdt.execs_to_run_in_order.begin(), cdt.execs_to_run_in_order.end());
            }
            to_destroy.insert(proc.first);
            MoveComponent(proc.first, cdt.last_entity, cdt.exec_outputs);
            MoveComponent(proc.first, cdt.last_entity, cdt.text_buffers[TextBufferType::kOutput]);
        }
    }
    for (const auto e: to_destroy) {
        DestroyEntity(e, cdt);
    }
}

static void OpenFileLink(Cdt& cdt) {
    if (!AcceptUsrCmd(kOpen, cdt.last_usr_cmd)) return;
    const auto exec_output = Find(cdt.last_entity, cdt.exec_outputs);
    if (cdt.open_in_editor_cmd.str.empty()) {
        WarnUserConfigPropNotSpecified(kOpenInEditorCommandProperty, cdt);
    } else if (!exec_output || exec_output->file_links.empty()) {
        cdt.os->Out() << kTcGreen << "No file links in the output" << kTcReset << std::endl;
    } else if (exec_output) {
        if (IsCmdArgInRange(cdt.last_usr_cmd, exec_output->file_links)) {
            const auto& link = exec_output->file_links[cdt.last_usr_cmd.arg - 1];
            const auto& shell_command = FormatTemplate(cdt.open_in_editor_cmd, link);
            cdt.os->ExecProcess(shell_command);
        } else {
            cdt.os->Out() << kTcGreen << "Last execution output:" << kTcReset << std::endl;
            for (const auto& line: cdt.text_buffers[TextBufferType::kOutput][cdt.last_entity]) {
                cdt.os->Out() << line << std::endl;
            }
        }
    }
}

static void SearchThroughLastExecutionOutput(Cdt& cdt) {
    if (!AcceptUsrCmd(kSearch, cdt.last_usr_cmd)) return;
    if (!Find(cdt.last_entity, cdt.exec_outputs)) {
        cdt.os->Out() << kTcGreen << "No task has been executed yet" << kTcReset << std::endl;
    } else {
        const auto& buffer = cdt.text_buffers[TextBufferType::kOutput][cdt.last_entity];
        cdt.text_buffer_searchs[cdt.last_entity] = TextBufferSearch{TextBufferType::kOutput, 0, buffer.size()};
    }
}

static void SearchThroughTextBuffer(Cdt& cdt) {
    std::unordered_set<Entity> to_destroy;
    for (const auto& it: cdt.text_buffer_searchs) {
        const auto input = ReadInputFromStdin("Regular expression: ", cdt);
        const auto& search = it.second;
        const auto& buffer = cdt.text_buffers[search.type][it.first];
        try {
            std::regex regex(input);
            bool results_found = false;
            for (auto i = search.search_start; i < search.search_end; i++) {
                const auto& line = buffer[i];
                const auto start = std::sregex_iterator(line.begin(), line.end(), regex);
                const auto end = std::sregex_iterator();
                std::unordered_set<size_t> highlight_starts, highlight_ends;
                for (auto it = start; it != end; it++) {
                    results_found = true;
                    highlight_starts.insert(it->position());
                    highlight_ends.insert(it->position() + it->length() - 1);
                }
                if (highlight_starts.empty()) {
                    continue;
                }
                cdt.os->Out() << kTcMagenta << i - search.search_start + 1 << ':' << kTcReset;
                for (auto j = 0; j < line.size(); j++) {
                    if (highlight_starts.count(j) > 0) {
                        cdt.os->Out() << kTcGreen;
                    }
                    cdt.os->Out() << line[j];
                    if (highlight_ends.count(j) > 0) {
                        cdt.os->Out() << kTcReset;
                    }
                }
                cdt.os->Out() << std::endl;
            }
            if (!results_found) {
                cdt.os->Out() << kTcGreen << "No matches found" << kTcReset << std::endl;
            }
        } catch (const std::regex_error& e) {
            cdt.os->Out() << kTcRed << "Invalid regular expression '" << input << "': " << e.what() << kTcReset << std::endl;
        }
        to_destroy.insert(it.first);
    }
    DestroyComponents(to_destroy, cdt.text_buffer_searchs);
}

static void ValidateIfDebuggerAvailable(Cdt& cdt) {
    if (!AcceptUsrCmd(kDebug, cdt.last_usr_cmd) && !AcceptUsrCmd(kGtestDebug, cdt.last_usr_cmd)) return;
    std::vector<std::string> mandatory_props_not_specified;
    if (cdt.debug_cmd.str.empty()) mandatory_props_not_specified.push_back(kDebugCommandProperty);
    if (cdt.execute_in_new_terminal_tab_cmd.str.empty()) mandatory_props_not_specified.push_back(kExecuteInNewTerminalTabCommandProperty);
    if (!mandatory_props_not_specified.empty()) {
        for (const auto& prop: mandatory_props_not_specified) {
            WarnUserConfigPropNotSpecified(prop, cdt);
        }
    } else {
        // Forward the command to be executed by the actual systems.
        cdt.last_usr_cmd.executed = false;
    }
}

static void StartNextExecutionWithDebugger(Cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    const auto exec_entity = cdt.execs_to_run_in_order.front();
    if (!Find(exec_entity, cdt.debug)) return;
    const auto& exec = cdt.execs[exec_entity];
    const auto debug_cmd = FormatTemplate(cdt.debug_cmd, exec.shell_command);
    const auto cmds_to_exec = "cd " + cdt.os->GetCurrentPath().string() + " && " + debug_cmd;
    const auto shell_cmd = FormatTemplate(cdt.execute_in_new_terminal_tab_cmd, cmds_to_exec);
    cdt.os->Out() << kTcMagenta << "Starting debugger for \"" << exec.name << "\"..." << kTcReset << std::endl;
    cdt.os->ExecProcess(shell_cmd);
    DestroyEntity(exec_entity, cdt);
}

static void PromptUserToAskForHelp(Cdt& cdt) {
    cdt.os->Out() << "Type " << kTcGreen << kHelp.cmd << kTcReset << " to see list of all the user commands." << std::endl;
}

static void DisplayHelp(const std::vector<UserCommandDefinition>& defs, Cdt& cdt) {
    if (cdt.last_usr_cmd.executed) return;
    cdt.last_usr_cmd.executed = true;
    cdt.os->Out() << kTcGreen << "User commands:" << kTcReset << std::endl;
    for (const auto& def: defs) {
        cdt.os->Out() << def.cmd;
        if (!def.arg.empty()) {
            cdt.os->Out() << '<' << def.arg << '>';
        }
        cdt.os->Out() << ((def.cmd.size() + def.arg.size()) < 6 ? "\t\t" : "\t") << def.desc << std::endl;
    }
}

bool InitCdt(int argc, const char **argv, Cdt &cdt) {
    global_cdt = &cdt;
    cdt.cdt_executable = argv[0];
    cdt.last_entity = CreateEntity(cdt);
    InitExampleUserConfig(cdt);
    ReadArgv(argc, argv, cdt);
    ReadUserConfig(cdt);
    if (PrintErrors(cdt)) return false;
    ReadTasksConfig(cdt);
    if (PrintErrors(cdt)) return false;
    ReadUserCommandFromEnv(cdt);
    PromptUserToAskForHelp(cdt);
    DisplayListOfTasks(cdt.tasks, cdt);
    cdt.os->Signal(SIGINT, TerminateCurrentExecutionOrExit);
    return true;
}

bool WillWaitForInput(Cdt &cdt) {
    return cdt.execs_to_run_in_order.empty() && cdt.processes.empty();
}

void ExecCdtSystems(Cdt &cdt) {
    ReadUserCommandFromStdin(cdt);
    ValidateIfDebuggerAvailable(cdt);
    ScheduleTask(cdt);
    OpenFileLink(cdt);
    SearchThroughLastExecutionOutput(cdt);
    DisplayGtestOutput(cdt);
    SearchThroughGtestOutput(cdt);
    RerunGtest(cdt);
    ScheduleGtestTaskWithFilter(cdt);
    DisplayHelp(kUsrCmdDefs, cdt);
    SearchThroughTextBuffer(cdt);
    ExecuteRestartTask(cdt);
    ScheduleGtestExecutions(cdt);
    ScheduleTaskExecutions(cdt);
    StartNextExecutionWithDebugger(cdt);
    StartNextExecution(cdt);
    ProcessExecutionEvent(cdt);
    ParseGtestOutput(cdt);
    DisplayGtestExecutionResult(cdt);
    StreamExecutionOutput(cdt);
    StreamExecutionOutputOnFailure(cdt);
    FindAndHighlightFileLinks(cdt);
    PrintExecutionOutput(cdt);
    DisplayExecutionResult(cdt);
    RestartRepeatingGtestOnSuccess(cdt);
    RestartRepeatingExecutionOnSuccess(cdt);
    FinishGtestExecution(cdt);
    FinishTaskExecution(cdt);
}
