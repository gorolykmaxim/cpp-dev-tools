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

const auto TC_RED = "\033[31m";
const auto TC_GREEN = "\033[32m";
const auto TC_BLUE = "\033[34m";
const auto TC_MAGENTA = "\033[35m";
const auto TC_RESET = "\033[0m";

const auto OPEN_IN_EDITOR_COMMAND_PROPERTY = "open_in_editor_command";
const auto EXECUTE_IN_NEW_TERMINAL_TAB_COMMAND_PROPERTY = "execute_in_new_terminal_tab_command";
const auto DEBUG_COMMAND_PROPERTY = "debug_command";
const auto ENV_VAR_LAST_COMMAND = "LAST_COMMAND";
const std::string TEMPLATE_ARG_PLACEHOLDER = "{}";
const std::string GTEST_TASK = "__gtest";
const std::string GTEST_FILTER_ARG = "--gtest_filter";

template <typename T>
T push_back_and_return(std::vector<T>& vec, T&& t) {
    vec.push_back(t);
    return t;
}

cdt* global_cdt;
std::vector<user_command_definition> USR_CMD_DEFS;
const auto TASK = push_back_and_return(USR_CMD_DEFS, {"t", "ind", "Execute the task with the specified index"});
const auto TASK_REPEAT = push_back_and_return(USR_CMD_DEFS, {"tr", "ind", "Keep executing the task with the specified index until it fails"});
const auto DEBUG = push_back_and_return(USR_CMD_DEFS, {"d", "ind", "Execute the task with the specified index with a debugger attached"});
const auto OPEN = push_back_and_return(USR_CMD_DEFS, {"o", "ind", "Open the file link with the specified index in your code editor"});
const auto SEARCH = push_back_and_return(USR_CMD_DEFS, {"s", "", "Search through output of the last executed task with the specified regular expression"});
const auto GTEST = push_back_and_return(USR_CMD_DEFS, {"g", "ind", "Display output of the specified google test"});
const auto GTEST_SEARCH = push_back_and_return(USR_CMD_DEFS, {"gs", "ind", "Search through output of the specified google test with the specified regular expression"});
const auto GTEST_RERUN = push_back_and_return(USR_CMD_DEFS, {"gt", "ind", "Re-run the google test with the specified index"});
const auto GTEST_RERUN_REPEAT = push_back_and_return(USR_CMD_DEFS, {"gtr", "ind", "Keep re-running the google test with the specified index until it fails"});
const auto GTEST_DEBUG = push_back_and_return(USR_CMD_DEFS, {"gd", "ind", "Re-run the google test with the specified index with debugger attached"});
const auto GTEST_FILTER = push_back_and_return(USR_CMD_DEFS, {"gf", "ind", "Run google tests of the task with the specified index with a specified " + GTEST_FILTER_ARG});
const auto HELP = push_back_and_return(USR_CMD_DEFS, {"h", "", "Display list of user commands"});

std::istream& os_api::in() {
    return std::cin;
}

std::ostream& os_api::out() {
    return std::cout;
}

std::ostream& os_api::err() {
    return std::cerr;
}

std::string os_api::get_env(const std::string &name) {
    const char* value = getenv(name.c_str());
    return value ? value : "";
}

void os_api::set_env(const std::string &name, const std::string &value) {
    setenv(name.c_str(), value.c_str(), true);
}

void os_api::set_current_path(const std::filesystem::path &path) {
    std::filesystem::current_path(path);
}

std::filesystem::path os_api::get_current_path() {
    return std::filesystem::current_path();
}

bool os_api::read_file(const std::filesystem::path &path, std::string &data) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    file >> std::noskipws;
    data = std::string(std::istream_iterator<char>(file), std::istream_iterator<char>());
    return true;
}

void os_api::write_file(const std::filesystem::path &path, const std::string &data) {
    std::ofstream file(path);
    file << data;
}

bool os_api::file_exists(const std::filesystem::path &path) {
    return std::filesystem::exists(path);
}

void os_api::signal(int signal, void (*handler)(int)) {
    std::signal(signal, handler);
}

void os_api::raise_signal(int signal) {
    std::raise(signal);
}

int os_api::exec(const std::vector<const char *> &args) {
    execvp(args[0], const_cast<char* const*>(args.data()));
    return errno;
}

void os_api::kill_process(entity e, std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>> &processes) {
    TinyProcessLib::Process::kill(processes[e]->get_id());
}

void os_api::exec_process(const std::string &shell_cmd) {
    TinyProcessLib::Process(shell_cmd).get_exit_status();
}

void os_api::start_process(entity e,
                           const std::string &shell_cmd,
                           const std::function<void (const char *, size_t)> &stdout_cb,
                           const std::function<void (const char *, size_t)> &stderr_cb,
                           const std::function<void ()> &exit_cb,
                           std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>> &processes) {
    processes[e] = std::make_unique<TinyProcessLib::Process>(shell_cmd, "", stdout_cb, stderr_cb, exit_cb);
}

int os_api::get_process_exit_code(entity e, std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>> &processes) {
    return processes[e]->get_exit_status();
}

static entity create_entity(cdt& cdt) {
    return cdt.entity_seed++;
}

static void destroy_entity(entity e, cdt& cdt) {
    cdt.execs_to_run_in_order.erase(std::remove(cdt.execs_to_run_in_order.begin(), cdt.execs_to_run_in_order.end(), e), cdt.execs_to_run_in_order.end());
    cdt.task_ids.erase(e);
    cdt.processes.erase(e);
    cdt.execs.erase(e);
    cdt.exec_outputs.erase(e);
    cdt.gtest_execs.erase(e);
    cdt.repeat_until_fail.erase(e);
    cdt.stream_output.erase(e);
    for (int t = text_buffer_type_process; t <= text_buffer_type_output; t++) {
        cdt.text_buffers[static_cast<text_buffer_type>(t)].erase(e);
    }
}

template <typename T>
static T* find(entity e, std::unordered_map<entity, T>& components) {
    auto res = components.find(e);
    return res != components.end() ? &res->second : nullptr;
}

static bool find(entity e, const std::unordered_set<entity>& components) {
    return components.count(e) > 0;
}


template <typename T>
static void move_component(entity source, entity target, std::unordered_map<entity, T>& components) {
    if (find(source, components)) {
        components[target] = std::move(components[source]);
        components.erase(source);
    }
}

template <typename T>
static void destroy_components(const std::unordered_set<entity>& es, std::unordered_map<entity, T>& components) {
    for (auto e: es) {
        components.erase(e);
    }
}

static void move_text_buffer(entity e, text_buffer_type from, text_buffer_type to, std::unordered_map<text_buffer_type, std::unordered_map<entity, std::vector<std::string>>>& text_buffers) {
    auto& f = text_buffers[from][e];
    auto& t = text_buffers[to][e];
    t.reserve(t.size() + f.size());
    t.insert(t.end(), f.begin(), f.end());
    f.clear();
}

static void warn_user_config_prop_not_specified(const std::string& property, cdt& cdt) {
    cdt.os->out() << TC_RED << '\'' << property << "' is not specified in " << cdt.user_config_path << TC_RESET << std::endl;
}

static std::string format_template(const template_string& templ, const std::string& str) {
    std::string res = templ.str;
    res.replace(templ.arg_pos, TEMPLATE_ARG_PLACEHOLDER.size(), str);
    return res;
}

static bool accept_usr_cmd(const user_command_definition& def, user_command& cmd) {
    if (!cmd.executed && def.cmd == cmd.cmd) {
        cmd.executed = true;
        return true;
    } else {
        return false;
    }
}

template <typename T>
static bool read_property(const nlohmann::json& json, const std::string& name, T& value,
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

static void pre_task_names_to_indexes(const std::vector<std::string>& pre_task_names, const std::vector<nlohmann::json>& tasks_json,
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

static bool read_argv(int argc, const char** argv, cdt& cdt) {
    if (argc < 2) {
        cdt.config_errors.emplace_back("usage: cpp-dev-tools tasks.json");
        return false;
    }
    cdt.tasks_config_path = std::filesystem::absolute(argv[1]);
    const auto& config_dir_path = cdt.tasks_config_path.parent_path();
    cdt.os->set_current_path(config_dir_path);
    return true;
}

static bool parse_json_file(const std::filesystem::path& config_path, bool should_exist, nlohmann::json& config_json,
                            std::vector<std::string>& errors, cdt& cdt) {
    std::string config_data;
    if (!cdt.os->read_file(config_path, config_data)) {
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

static void append_config_errors(const std::filesystem::path& config_path, const std::vector<std::string>& from,
                                 std::vector<std::string>& to) {
    if (!from.empty()) {
        to.emplace_back(config_path.string() + " is invalid:");
        to.insert(to.end(), from.begin(), from.end());
    }
}

static void init_example_user_config(cdt& cdt) {
    cdt.user_config_path = std::filesystem::path(cdt.os->get_env("HOME")) / ".cpp-dev-tools.json";
    if (!cdt.os->file_exists(cdt.user_config_path)) {
        std::stringstream example;
        example << "{" << std::endl;
        example << "  // Open file links from the output in Sublime Text:" << std::endl;
        example << "  //\"" << OPEN_IN_EDITOR_COMMAND_PROPERTY << "\": \"subl {}\"" << std::endl;
        example << "  // Open file links from the output in VSCode:" << std::endl;
        example << "  //\"" << OPEN_IN_EDITOR_COMMAND_PROPERTY << "\": \"code {}\"" << std::endl;
        example << "  // Execute in a new terminal tab on MacOS:" << std::endl;
        example << "  // \"" << EXECUTE_IN_NEW_TERMINAL_TAB_COMMAND_PROPERTY << "\": \"osascript -e 'tell application \\\"Terminal\\\" to do script \\\"{}\\\"'\"" << std::endl;
        example << "  // Execute in a new terminal tab on Windows:" << std::endl;
        example << "  // \"" << EXECUTE_IN_NEW_TERMINAL_TAB_COMMAND_PROPERTY << "\": \"/c/Program\\ Files/Git/git-bash -c '{}'\"" << std::endl;
        example << "  // Debug tasks via lldb:" << std::endl;
        example << "  //\"" << DEBUG_COMMAND_PROPERTY << "\": \"lldb -- {}\"" << std::endl;
        example << "  // Debug tasks via gdb:" << std::endl;
        example << "  //\"" << DEBUG_COMMAND_PROPERTY << "\": \"gdb --args {}\"" << std::endl;
        example << "}" << std::endl;
        cdt.os->write_file(cdt.user_config_path, example.str());
    }
}

static std::function<bool()> parse_template_string(template_string& temp_str) {
    return [&temp_str] () {
        const auto pos = temp_str.str.find(TEMPLATE_ARG_PLACEHOLDER);
        if (pos == std::string::npos) {
            return false;
        } else {
            temp_str.arg_pos = pos;
            return true;
        }
    };
}

static void read_user_config(cdt& cdt) {
    static const std::string err_suffix = "must be a string in format, examples of which you can find in the config";
    nlohmann::json config_json;
    if (!parse_json_file(cdt.user_config_path, false, config_json, cdt.config_errors, cdt)) {
        return;
    }
    std::vector<std::string> config_errors;
    read_property(config_json, OPEN_IN_EDITOR_COMMAND_PROPERTY, cdt.open_in_editor_cmd.str, true, "", err_suffix, config_errors, parse_template_string(cdt.open_in_editor_cmd));
    read_property(config_json, DEBUG_COMMAND_PROPERTY, cdt.debug_cmd.str, true, "", err_suffix, config_errors, parse_template_string(cdt.debug_cmd));
    read_property(config_json, EXECUTE_IN_NEW_TERMINAL_TAB_COMMAND_PROPERTY, cdt.execute_in_new_terminal_tab_cmd.str, true, "", err_suffix, config_errors, parse_template_string(cdt.execute_in_new_terminal_tab_cmd));
    append_config_errors(cdt.user_config_path, config_errors, cdt.config_errors);
}

static void read_tasks_config(cdt& cdt) {
    nlohmann::json config_json;
    std::vector<std::string> config_errors;
    if (!parse_json_file(cdt.tasks_config_path, true, config_json, cdt.config_errors, cdt)) {
        return;
    }
    std::vector<nlohmann::json> tasks_json;
    if (!read_property(config_json, "cdt_tasks", tasks_json, false, "", "must be an array of task objects", config_errors)) {
        append_config_errors(cdt.tasks_config_path, config_errors, cdt.config_errors);
        return;
    }
    std::vector<std::vector<size_t>> direct_pre_tasks(tasks_json.size());
    cdt.pre_tasks = std::vector<std::vector<size_t>>(tasks_json.size());
    // Initialize tasks with their direct "pre_tasks" dependencies
    for (auto i = 0; i < tasks_json.size(); i++) {
        task new_task;
        const auto& task_json = tasks_json[i];
        const auto err_prefix = "task #" + std::to_string(i + 1) + ": ";
        read_property(task_json, "name", new_task.name, false, err_prefix, "must be a string", config_errors);
        read_property(task_json, "command", new_task.command, false, err_prefix, "must be a string", config_errors);
        std::vector<std::string> pre_task_names;
        if (read_property(task_json, "pre_tasks", pre_task_names, true, err_prefix, "must be an array of other task names", config_errors)) {
            pre_task_names_to_indexes(pre_task_names, tasks_json, direct_pre_tasks[i], err_prefix, config_errors);
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
    append_config_errors(cdt.tasks_config_path, config_errors, cdt.config_errors);
}

static bool print_errors(const cdt& cdt) {
    if (cdt.config_errors.empty()) return false;
    cdt.os->err() << TC_RED;
    for (const auto& e: cdt.config_errors) {
        cdt.os->err() << e << std::endl;
    }
    cdt.os->err() << TC_RESET;
    return true;
}

static user_command parse_user_command(const std::string& str) {
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
    user_command cmd;
    cmd.cmd = chars.str();
    cmd.arg = std::atoi(digits.str().c_str());
    return cmd;
}

template<typename T>
static bool is_cmd_arg_in_range(const user_command& cmd, const std::vector<T>& range) {
    return cmd.arg > 0 && cmd.arg <= range.size();
}

static std::string read_input_from_stdin(const std::string& prefix, cdt& cdt) {
    cdt.os->out() << TC_GREEN << prefix << TC_RESET;
    std::string input;
    std::getline(cdt.os->in(), input);
    return input;
}

static void read_user_command_from_stdin(cdt& cdt) {
    if (!will_wait_for_input(cdt)) return;
    const auto input = read_input_from_stdin("(cdt) ", cdt);
    if (input.empty()) {
        cdt.last_usr_cmd.executed = false;
    } else {
        cdt.last_usr_cmd = parse_user_command(input);
    }
}

static void read_user_command_from_env(cdt& cdt) {
    const auto str = cdt.os->get_env(ENV_VAR_LAST_COMMAND);
    if (!str.empty()) {
        cdt.last_usr_cmd = parse_user_command(str);
    }
}

static void display_list_of_tasks(const std::vector<task>& tasks, cdt& cdt) {
    cdt.os->out() << TC_GREEN << "Tasks:" << TC_RESET << std::endl;
    for (auto i = 0; i < tasks.size(); i++) {
        cdt.os->out() << i + 1 << " \"" << tasks[i].name << "\"" << std::endl;
    }
}

static void schedule_task(cdt& cdt) {
    if (!accept_usr_cmd(TASK, cdt.last_usr_cmd) && !accept_usr_cmd(TASK_REPEAT, cdt.last_usr_cmd) && !accept_usr_cmd(DEBUG, cdt.last_usr_cmd)) return;
    if (!is_cmd_arg_in_range(cdt.last_usr_cmd, cdt.tasks)) {
        display_list_of_tasks(cdt.tasks, cdt);
    } else {
        const auto id = cdt.last_usr_cmd.arg - 1;
        for (const auto pre_task_id: cdt.pre_tasks[id]) {
            const auto pre_task_exec = create_entity(cdt);
            cdt.task_ids[pre_task_exec] = pre_task_id;
            cdt.execs_to_run_in_order.push_back(pre_task_exec);
        }
        const auto task_exec = create_entity(cdt);
        cdt.task_ids[task_exec] = id;
        cdt.stream_output.insert(task_exec);
        if (TASK_REPEAT.cmd == cdt.last_usr_cmd.cmd) {
            cdt.repeat_until_fail.insert(task_exec);
        }
        if (DEBUG.cmd == cdt.last_usr_cmd.cmd) {
            cdt.debug.insert(task_exec);
        }
        cdt.execs_to_run_in_order.push_back(task_exec);
    }
}

static void terminate_current_execution_or_exit(int signal) {
    if (global_cdt->processes.empty()) {
        global_cdt->os->signal(signal, SIG_DFL);
        global_cdt->os->raise_signal(signal);
    } else {
        for (const auto& [e, _]: global_cdt->processes) {
            global_cdt->os->kill_process(e, global_cdt->processes);
        }
    }
}

static void execute_restart_task(cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    const auto exec = cdt.execs_to_run_in_order.front();
    if (cdt.tasks[cdt.task_ids[exec]].command != "__restart") return;
    destroy_entity(exec, cdt);
    cdt.os->out() << TC_MAGENTA << "Restarting program..." << TC_RESET << std::endl;
    const auto cmd_str = cdt.last_usr_cmd.cmd + std::to_string(cdt.last_usr_cmd.arg);
    cdt.os->set_env(ENV_VAR_LAST_COMMAND, cmd_str);
    int error = cdt.os->exec({cdt.cdt_executable, cdt.tasks_config_path.c_str(), nullptr});
    if (error) {
        cdt.os->out() << TC_RED << "Failed to restart: " << std::strerror(error) << TC_RESET << std::endl;
    }
}

static std::string gtest_task_to_shell_command(const task& task, const std::optional<std::string>& gtest_filter = {}) {
    auto binary = task.command.substr(GTEST_TASK.size() + 1);
    if (gtest_filter) {
        binary += " " + GTEST_FILTER_ARG + "='" + *gtest_filter + "'";
    }
    return binary;
}

static void schedule_gtest_executions(cdt& cdt) {
    for (const auto exec: cdt.execs_to_run_in_order) {
        const auto& task = cdt.tasks[cdt.task_ids[exec]];
        if (task.command.find(GTEST_TASK) == 0) {
            if (!find(exec, cdt.execs)) {
                cdt.execs[exec] = execution{task.name, gtest_task_to_shell_command(task)};
            }
            if (!find(exec, cdt.gtest_execs)) {
                cdt.gtest_execs[exec] = gtest_execution{};
            }
        }
    }
}

static void schedule_task_executions(cdt& cdt) {
    for (const auto exec: cdt.execs_to_run_in_order) {
        if (!find(exec, cdt.execs)) {
            const auto& task = cdt.tasks[cdt.task_ids[exec]];
            cdt.execs[exec] = execution{task.name, task.command};
        }
    }
}

static std::function<void(const char*,size_t)> write_to(moodycamel::BlockingConcurrentQueue<execution_event>& queue, entity exec, execution_event_type event_type) {
    return [&queue, exec, event_type] (const char* data, size_t size) {
        execution_event event;
        event.exec = exec;
        event.type = event_type;
        event.data = std::string(data, size);
        queue.enqueue(event);
    };
}

static std::function<void()> handle_exit(moodycamel::BlockingConcurrentQueue<execution_event>& queue, entity exec) {
    return [&queue, exec] () {
        execution_event event;
        event.exec = exec;
        event.type = execution_event_type_exit;
        queue.enqueue(event);
    };
}

static void start_next_execution(cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    const auto exec_entity = cdt.execs_to_run_in_order.front();
    const auto& exec = cdt.execs[exec_entity];
    cdt.os->start_process(
        exec_entity,
        exec.shell_command,
        write_to(cdt.exec_event_queue, exec_entity, execution_event_type_stdout),
        write_to(cdt.exec_event_queue, exec_entity, execution_event_type_stderr),
        handle_exit(cdt.exec_event_queue, exec_entity),
        cdt.processes
    );
    cdt.exec_outputs[exec_entity] = execution_output{};
    if (find(exec_entity, cdt.stream_output)) {
        cdt.os->out() << TC_MAGENTA << "Running \"" << exec.name << "\"" << TC_RESET << std::endl;
    } else {
        cdt.os->out() << TC_BLUE << "Running \"" << exec.name << "\"..." << TC_RESET << std::endl;
    }
}

static void process_execution_event(cdt& cdt) {
    if (cdt.processes.empty()) return;
    execution_event event;
    cdt.exec_event_queue.wait_dequeue(event);
    auto& exec = cdt.execs[event.exec];
    auto& buffer = cdt.text_buffers[text_buffer_type_process][event.exec];
    if (event.type == execution_event_type_exit) {
        exec.state = cdt.os->get_process_exit_code(event.exec, cdt.processes) == 0 ? execution_state_complete : execution_state_failed;
    } else {
        auto& line_buffer = event.type == execution_event_type_stdout ? exec.stdout_line_buffer : exec.stderr_line_buffer;
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

static void complete_current_gtest(gtest_execution& gtest_exec, const std::string& line_content, bool& test_completed) {
    gtest_exec.tests[*gtest_exec.current_test].duration = line_content.substr(line_content.rfind('('));
    gtest_exec.current_test.reset();
    test_completed = true;
}

static void parse_gtest_output(cdt& cdt) {
    static const auto test_count_index = std::string("Running ").size();
    for (const auto& proc: cdt.processes) {
        auto& proc_buffer = cdt.text_buffers[text_buffer_type_process][proc.first];
        auto& gtest_buffer = cdt.text_buffers[text_buffer_type_gtest][proc.first];
        auto& out_buffer = cdt.text_buffers[text_buffer_type_output][proc.first];
        auto& exec_output = cdt.exec_outputs[proc.first];
        const auto gtest_exec = find(proc.first, cdt.gtest_execs);
        const auto stream_output = find(proc.first, cdt.stream_output);
        if (!gtest_exec) {
            continue;
        }
        if (gtest_exec->state == gtest_execution_state_running || gtest_exec->state == gtest_execution_state_parsing) {
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
                    complete_current_gtest(*gtest_exec, line_content, test_completed);
                } else if (found_word == "FAILED" && line_content.find(gtest_exec->tests.back().name) == 0) {
                    gtest_exec->failed_test_ids.push_back(*gtest_exec->current_test);
                    complete_current_gtest(*gtest_exec, line_content, test_completed);
                } else if (gtest_exec->current_test) {
                    gtest_exec->tests[*gtest_exec->current_test].buffer_end++;
                    gtest_buffer.push_back(line);
                    if (stream_output && gtest_exec->rerun_of_single_test) {
                        out_buffer.push_back(line);
                    }
                } else if (found_word == "RUN") {
                    gtest_exec->current_test = gtest_exec->tests.size();
                    gtest_test test{line_content};
                    test.buffer_start = gtest_buffer.size();
                    test.buffer_end = test.buffer_start;
                    gtest_exec->tests.push_back(test);
                } else if (filler_char == '=') {
                    if (gtest_exec->state == gtest_execution_state_running) {
                        const auto count_end_index = line_content.find(' ', test_count_index);
                        const auto count_str = line_content.substr(test_count_index, count_end_index - test_count_index);
                        gtest_exec->test_count = std::stoi(count_str);
                        gtest_exec->tests.reserve(gtest_exec->test_count);
                        gtest_exec->state = gtest_execution_state_parsing;
                    } else {
                        gtest_exec->total_duration = line_content.substr(line_content.rfind('('));
                        gtest_exec->state = gtest_execution_state_parsed;
                        break;
                    }
                }
                if (stream_output && !gtest_exec->rerun_of_single_test && test_completed) {
                    cdt.os->out() << std::flush << "\rTests completed: " << gtest_exec->tests.size() << " of " << gtest_exec->test_count;
                }
            }
        }
        proc_buffer.clear();
    }
}

static void print_gtest_list(const std::vector<size_t>& test_ids, const std::vector<gtest_test>& tests, cdt& cdt) {
    for (auto i = 0; i < test_ids.size(); i++) {
        const auto id = test_ids[i];
        const auto& test = tests[id];
        cdt.os->out() << i + 1 << " \"" << test.name << "\" " << test.duration << std::endl;
    }
}

static void print_failed_gtest_list(const gtest_execution& exec, cdt& cdt) {
    cdt.os->out() << TC_RED << "Failed tests:" << TC_RESET << std::endl;
    print_gtest_list(exec.failed_test_ids, exec.tests, cdt);
    const int failed_percent = exec.failed_test_ids.size() / (float)exec.tests.size() * 100;
    cdt.os->out() << TC_RED << "Tests failed: " << exec.failed_test_ids.size() << " of " << exec.tests.size() << " (" << failed_percent << "%) " << exec.total_duration << TC_RESET << std::endl;
}

static void print_gtest_output(execution_output& out, const std::vector<std::string>& gtest_buffer, std::vector<std::string>& out_buffer, const gtest_test& test, const std::string& color) {
    out = execution_output{};
    out_buffer.clear();
    out_buffer.reserve(test.buffer_end - test.buffer_start + 1);
    out_buffer.emplace_back(color + "\"" + test.name + "\" output:" + TC_RESET);
    out_buffer.insert(out_buffer.end(), gtest_buffer.begin() + test.buffer_start, gtest_buffer.begin() + test.buffer_end);
}

static gtest_test* find_gtest_by_cmd_arg(const user_command& cmd, gtest_execution* exec, cdt& cdt) {
    if (!exec || exec->test_count == 0) {
        cdt.os->out() << TC_GREEN << "No google tests have been executed yet." << TC_RESET << std::endl;
    } else if (exec->failed_test_ids.empty()) {
        if (!is_cmd_arg_in_range(cmd, exec->tests)) {
            cdt.os->out() << TC_GREEN << "Last executed tests " << exec->total_duration << ":" << TC_RESET << std::endl;
            std::vector<size_t> ids(exec->tests.size());
            std::iota(ids.begin(), ids.end(), 0);
            print_gtest_list(ids, exec->tests, cdt);
        } else {
            return &exec->tests[cmd.arg - 1];
        }
    } else {
        if (!is_cmd_arg_in_range(cmd, exec->failed_test_ids)) {
            print_failed_gtest_list(*exec, cdt);
        } else {
            return &exec->tests[exec->failed_test_ids[cmd.arg - 1]];
        }
    }
    return nullptr;
}

static void display_gtest_output(cdt& cdt) {
    if (!accept_usr_cmd(GTEST, cdt.last_usr_cmd)) return;
    const auto last_gtest_exec = find(cdt.last_entity, cdt.gtest_execs);
    const auto last_exec_output = find(cdt.last_entity, cdt.exec_outputs);
    const auto& gtest_buffer = cdt.text_buffers[text_buffer_type_gtest][cdt.last_entity];
    auto& out_buffer = cdt.text_buffers[text_buffer_type_output][cdt.last_entity];
    const auto test = find_gtest_by_cmd_arg(cdt.last_usr_cmd, last_gtest_exec, cdt);
    if (last_gtest_exec && last_exec_output && test) {
        print_gtest_output(*last_exec_output, gtest_buffer, out_buffer, *test, last_gtest_exec->failed_test_ids.empty() ? TC_GREEN : TC_RED);
    }
}

static void search_through_gtest_output(cdt& cdt) {
    if (!accept_usr_cmd(GTEST_SEARCH, cdt.last_usr_cmd)) return;
    const auto last_gtest_exec = find(cdt.last_entity, cdt.gtest_execs);
    const auto test = find_gtest_by_cmd_arg(cdt.last_usr_cmd, last_gtest_exec, cdt);
    if (test) {
        cdt.text_buffer_searchs[cdt.last_entity] = text_buffer_search{text_buffer_type_gtest, test->buffer_start, test->buffer_end};
    }
}

static void rerun_gtest(cdt& cdt) {
    if (!accept_usr_cmd(GTEST_RERUN, cdt.last_usr_cmd) && !accept_usr_cmd(GTEST_RERUN_REPEAT, cdt.last_usr_cmd) && !accept_usr_cmd(GTEST_DEBUG, cdt.last_usr_cmd)) return;
    const auto last_gtest_exec = find(cdt.last_entity, cdt.gtest_execs);
    const auto gtest_task_id = find(cdt.last_entity, cdt.task_ids);
    const auto test = find_gtest_by_cmd_arg(cdt.last_usr_cmd, last_gtest_exec, cdt);
    if (last_gtest_exec && gtest_task_id && test) {
        for (const auto& pre_task_id: cdt.pre_tasks[*gtest_task_id]) {
            const auto pre_task_exec = create_entity(cdt);
            cdt.task_ids[pre_task_exec] = pre_task_id;
            cdt.execs_to_run_in_order.push_back(pre_task_exec);
        }
        const auto exec = create_entity(cdt);
        cdt.task_ids[exec] = *gtest_task_id;
        cdt.execs[exec] = execution{test->name, gtest_task_to_shell_command(cdt.tasks[*gtest_task_id], test->name)};
        cdt.gtest_execs[exec] = gtest_execution{true};
        cdt.stream_output.insert(exec);
        if (GTEST_RERUN_REPEAT.cmd == cdt.last_usr_cmd.cmd) {
            cdt.repeat_until_fail.insert(exec);
        }
        if (GTEST_DEBUG.cmd == cdt.last_usr_cmd.cmd) {
            cdt.debug.insert(exec);
        }
        cdt.execs_to_run_in_order.push_back(exec);
    }
}

static void schedule_gtest_task_with_filter(cdt& cdt) {
    if (!accept_usr_cmd(GTEST_FILTER, cdt.last_usr_cmd)) return;
    if (!is_cmd_arg_in_range(cdt.last_usr_cmd, cdt.tasks)) {
        display_list_of_tasks(cdt.tasks, cdt);
    } else {
        const auto filter = read_input_from_stdin(GTEST_FILTER_ARG + "=", cdt);
        const auto task_id = cdt.last_usr_cmd.arg - 1;
        for (const auto pre_task_id: cdt.pre_tasks[task_id]) {
            const auto pre_task_exec = create_entity(cdt);
            cdt.task_ids[pre_task_exec] = pre_task_id;
            cdt.execs_to_run_in_order.push_back(pre_task_exec);
        }
        const auto exec = create_entity(cdt);
        cdt.task_ids[exec] = task_id;
        cdt.execs[exec] = execution{filter, gtest_task_to_shell_command(cdt.tasks[task_id], filter)};
        cdt.stream_output.insert(exec);
        cdt.execs_to_run_in_order.push_back(exec);
    }
}

static void display_gtest_execution_result(cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        const auto task_id = cdt.task_ids[proc.first];
        auto& exec = cdt.execs[proc.first];
        auto& exec_output = cdt.exec_outputs[proc.first];
        const auto gtest_exec = find(proc.first, cdt.gtest_execs);
        if (gtest_exec && !gtest_exec->rerun_of_single_test && exec.state != execution_state_running && gtest_exec->state != gtest_execution_state_finished) {
            cdt.os->out() << "\33[2K\r"; // Current line has test execution progress displayed. We will start displaying our output on top of it.
            if (gtest_exec->state == gtest_execution_state_running) {
                exec.state = execution_state_failed;
                cdt.os->out() << TC_RED << "'" << gtest_task_to_shell_command(cdt.tasks[task_id]) << "' is not a google test executable" << TC_RESET << std::endl;
            } else if (gtest_exec->state == gtest_execution_state_parsing) {
                exec.state = execution_state_failed;
                cdt.os->out() << TC_RED << "Tests have finished prematurely" << TC_RESET << std::endl;
                // Tests might have crashed in the middle of some test. If so - consider test failed.
                // This is not always true however: tests might have crashed in between two test cases.
                if (gtest_exec->current_test) {
                    gtest_exec->failed_test_ids.push_back(*gtest_exec->current_test);
                    gtest_exec->current_test.reset();
                }
            } else if (gtest_exec->failed_test_ids.empty() && find(proc.first, cdt.stream_output)) {
                cdt.os->out() << TC_GREEN << "Successfully executed " << gtest_exec->tests.size() << " tests "  << gtest_exec->total_duration << TC_RESET << std::endl;
            }
            if (!gtest_exec->failed_test_ids.empty()) {
                print_failed_gtest_list(*gtest_exec, cdt);
                if (gtest_exec->failed_test_ids.size() == 1) {
                    const auto& test = gtest_exec->tests[gtest_exec->failed_test_ids[0]];
                    const auto& gtest_buffer = cdt.text_buffers[text_buffer_type_gtest][proc.first];
                    auto& out_buffer = cdt.text_buffers[text_buffer_type_output][proc.first];
                    print_gtest_output(exec_output, gtest_buffer, out_buffer, test, TC_RED);
                }
            }
            gtest_exec->state = gtest_execution_state_finished;
        }
    }
}

static void restart_repeating_gtest_on_success(cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        const auto gtest_exec = find(proc.first, cdt.gtest_execs);
        if (gtest_exec && cdt.execs[proc.first].state == execution_state_complete && find(proc.first, cdt.repeat_until_fail)) {
            cdt.gtest_execs[proc.first] = gtest_execution{gtest_exec->rerun_of_single_test};
            cdt.text_buffers[text_buffer_type_gtest][proc.first].clear();
        }
    }
}

static void finish_gtest_execution(cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        const auto gtest_exec = find(proc.first, cdt.gtest_execs);
        if (gtest_exec && cdt.execs[proc.first].state != execution_state_running && !gtest_exec->rerun_of_single_test) {
            move_component(proc.first, cdt.last_entity, cdt.task_ids);
            move_component(proc.first, cdt.last_entity, cdt.text_buffers[text_buffer_type_gtest]);
            move_component(proc.first, cdt.last_entity, cdt.gtest_execs);
        }
    }
}

static void stream_execution_output(cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        if (find(proc.first, cdt.stream_output)) {
            move_text_buffer(proc.first, text_buffer_type_process, text_buffer_type_output, cdt.text_buffers);
        }
    }
}

static void find_and_highlight_file_links(cdt& cdt) {
    static const std::regex FILE_LINK_REGEX("(\\/[^:]+):([0-9]+):?([0-9]+)?");
    if (cdt.open_in_editor_cmd.str.empty()) return;
    for (auto& out: cdt.exec_outputs) {
        auto& buffer = cdt.text_buffers[text_buffer_type_output][out.first];
        for (auto i = out.second.lines_processed; i < buffer.size(); i++) {
            auto& line = buffer[i];
            std::stringstream highlighted_line;
            auto last_link_end_pos = 0;
            for (auto it = std::sregex_iterator(line.begin(), line.end(), FILE_LINK_REGEX); it != std::sregex_iterator(); it++) {
                std::stringstream link_stream;
                link_stream << (*it)[1] << ':' << (*it)[2];
                if (it->size() > 3 && (*it)[3].matched) {
                    link_stream << ':' << (*it)[3];
                }
                const auto link = link_stream.str();
                out.second.file_links.push_back(link);
                highlighted_line << line.substr(last_link_end_pos, it->position() - last_link_end_pos) << TC_MAGENTA << '[' << OPEN.cmd << out.second.file_links.size() << "] " << link << TC_RESET;
                last_link_end_pos = it->position() + it->length();
            }
            highlighted_line << line.substr(last_link_end_pos, line.size() - last_link_end_pos);
            line = highlighted_line.str();
        }
    }
}

static void print_execution_output(cdt& cdt) {
    for (auto& out: cdt.exec_outputs) {
        auto& buffer = cdt.text_buffers[text_buffer_type_output][out.first];
        for (auto i = out.second.lines_processed; i < buffer.size(); i++) {
            cdt.os->out() << buffer[i] << std::endl;
        }
        out.second.lines_processed = buffer.size();
    }
}

static void stream_execution_output_on_failure(cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        if (cdt.execs[proc.first].state == execution_state_failed && !find(proc.first, cdt.stream_output)) {
            move_text_buffer(proc.first, text_buffer_type_process, text_buffer_type_output, cdt.text_buffers);
        }
    }
}

static void display_execution_result(cdt& cdt) {
    for (auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state != execution_state_running) {
            const auto code = cdt.os->get_process_exit_code(proc.first, cdt.processes);
            if (exec.state == execution_state_failed) {
                cdt.os->out() << TC_RED << "'" << exec.name << "' failed: return code: " << code << TC_RESET << std::endl;
            } else if (find(proc.first, cdt.stream_output)) {
                cdt.os->out() << TC_MAGENTA << "'" << exec.name << "' complete: return code: " << code << TC_RESET << std::endl;
            }
        }
    }
}

static void restart_repeating_execution_on_success(cdt& cdt) {
    std::unordered_set<entity> to_destroy;
    for (const auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state == execution_state_complete && find(proc.first, cdt.repeat_until_fail)) {
            cdt.execs[proc.first] = execution{exec.name, exec.shell_command};
            cdt.text_buffers[text_buffer_type_output][proc.first].clear();
            to_destroy.insert(proc.first);
        }
    }
    destroy_components(to_destroy, cdt.processes);
}

static void finish_task_execution(cdt& cdt) {
    std::unordered_set<entity> to_destroy;
    for (auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state != execution_state_running) {
            if (exec.state == execution_state_failed) {
                to_destroy.insert(cdt.execs_to_run_in_order.begin(), cdt.execs_to_run_in_order.end());
            }
            to_destroy.insert(proc.first);
            move_component(proc.first, cdt.last_entity, cdt.exec_outputs);
            move_component(proc.first, cdt.last_entity, cdt.text_buffers[text_buffer_type_output]);
        }
    }
    for (const auto e: to_destroy) {
        destroy_entity(e, cdt);
    }
}

static void open_file_link(cdt& cdt) {
    if (!accept_usr_cmd(OPEN, cdt.last_usr_cmd)) return;
    const auto exec_output = find(cdt.last_entity, cdt.exec_outputs);
    if (cdt.open_in_editor_cmd.str.empty()) {
        warn_user_config_prop_not_specified(OPEN_IN_EDITOR_COMMAND_PROPERTY, cdt);
    } else if (!exec_output || exec_output->file_links.empty()) {
        cdt.os->out() << TC_GREEN << "No file links in the output" << TC_RESET << std::endl;
    } else if (exec_output) {
        if (is_cmd_arg_in_range(cdt.last_usr_cmd, exec_output->file_links)) {
            const auto& link = exec_output->file_links[cdt.last_usr_cmd.arg - 1];
            const auto& shell_command = format_template(cdt.open_in_editor_cmd, link);
            cdt.os->exec_process(shell_command);
        } else {
            cdt.os->out() << TC_GREEN << "Last execution output:" << TC_RESET << std::endl;
            for (const auto& line: cdt.text_buffers[text_buffer_type_output][cdt.last_entity]) {
                cdt.os->out() << line << std::endl;
            }
        }
    }
}

static void search_through_last_execution_output(cdt& cdt) {
    if (!accept_usr_cmd(SEARCH, cdt.last_usr_cmd)) return;
    if (!find(cdt.last_entity, cdt.exec_outputs)) {
        cdt.os->out() << TC_GREEN << "No task has been executed yet" << TC_RESET << std::endl;
    } else {
        const auto& buffer = cdt.text_buffers[text_buffer_type_output][cdt.last_entity];
        cdt.text_buffer_searchs[cdt.last_entity] = text_buffer_search{text_buffer_type_output, 0, buffer.size()};
    }
}

static void search_through_text_buffer(cdt& cdt) {
    std::unordered_set<entity> to_destroy;
    for (const auto& it: cdt.text_buffer_searchs) {
        const auto input = read_input_from_stdin("Regular expression: ", cdt);
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
                cdt.os->out() << TC_MAGENTA << i - search.search_start + 1 << ':' << TC_RESET;
                for (auto j = 0; j < line.size(); j++) {
                    if (highlight_starts.count(j) > 0) {
                        cdt.os->out() << TC_GREEN;
                    }
                    cdt.os->out() << line[j];
                    if (highlight_ends.count(j) > 0) {
                        cdt.os->out() << TC_RESET;
                    }
                }
                cdt.os->out() << std::endl;
            }
            if (!results_found) {
                cdt.os->out() << TC_GREEN << "No matches found" << TC_RESET << std::endl;
            }
        } catch (const std::regex_error& e) {
            cdt.os->out() << TC_RED << "Invalid regular expression '" << input << "': " << e.what() << TC_RESET << std::endl;
        }
        to_destroy.insert(it.first);
    }
    destroy_components(to_destroy, cdt.text_buffer_searchs);
}

static void validate_if_debugger_available(cdt& cdt) {
    if (!accept_usr_cmd(DEBUG, cdt.last_usr_cmd) && !accept_usr_cmd(GTEST_DEBUG, cdt.last_usr_cmd)) return;
    std::vector<std::string> mandatory_props_not_specified;
    if (cdt.debug_cmd.str.empty()) mandatory_props_not_specified.push_back(DEBUG_COMMAND_PROPERTY);
    if (cdt.execute_in_new_terminal_tab_cmd.str.empty()) mandatory_props_not_specified.push_back(EXECUTE_IN_NEW_TERMINAL_TAB_COMMAND_PROPERTY);
    if (!mandatory_props_not_specified.empty()) {
        for (const auto& prop: mandatory_props_not_specified) {
            warn_user_config_prop_not_specified(prop, cdt);
        }
    } else {
        // Forward the command to be executed by the actual systems.
        cdt.last_usr_cmd.executed = false;
    }
}

static void start_next_execution_with_debugger(cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    const auto exec_entity = cdt.execs_to_run_in_order.front();
    if (!find(exec_entity, cdt.debug)) return;
    const auto& exec = cdt.execs[exec_entity];
    const auto debug_cmd = format_template(cdt.debug_cmd, exec.shell_command);
    const auto cmds_to_exec = "cd " + cdt.os->get_current_path().string() + " && " + debug_cmd;
    const auto shell_cmd = format_template(cdt.execute_in_new_terminal_tab_cmd, cmds_to_exec);
    cdt.os->out() << TC_MAGENTA << "Starting debugger for \"" << exec.name << "\"..." << TC_RESET << std::endl;
    cdt.os->exec_process(shell_cmd);
    destroy_entity(exec_entity, cdt);
}

static void prompt_user_to_ask_for_help(cdt& cdt) {
    cdt.os->out() << "Type " << TC_GREEN << HELP.cmd << TC_RESET << " to see list of all the user commands." << std::endl;
}

static void display_help(const std::vector<user_command_definition>& defs, cdt& cdt) {
    if (cdt.last_usr_cmd.executed) return;
    cdt.last_usr_cmd.executed = true;
    cdt.os->out() << TC_GREEN << "User commands:" << TC_RESET << std::endl;
    for (const auto& def: defs) {
        cdt.os->out() << def.cmd;
        if (!def.arg.empty()) {
            cdt.os->out() << '<' << def.arg << '>';
        }
        cdt.os->out() << ((def.cmd.size() + def.arg.size()) < 6 ? "\t\t" : "\t") << def.desc << std::endl;
    }
}

bool init_cdt(int argc, const char **argv, cdt &cdt) {
    global_cdt = &cdt;
    cdt.cdt_executable = argv[0];
    cdt.last_entity = create_entity(cdt);
    init_example_user_config(cdt);
    read_argv(argc, argv, cdt);
    read_user_config(cdt);
    if (print_errors(cdt)) return false;
    read_tasks_config(cdt);
    if (print_errors(cdt)) return false;
    read_user_command_from_env(cdt);
    prompt_user_to_ask_for_help(cdt);
    display_list_of_tasks(cdt.tasks, cdt);
    cdt.os->signal(SIGINT, terminate_current_execution_or_exit);
    return true;
}

bool will_wait_for_input(cdt &cdt) {
    return cdt.execs_to_run_in_order.empty() && cdt.processes.empty();
}

void exec_cdt_systems(cdt &cdt) {
    read_user_command_from_stdin(cdt);
    validate_if_debugger_available(cdt);
    schedule_task(cdt);
    open_file_link(cdt);
    search_through_last_execution_output(cdt);
    display_gtest_output(cdt);
    search_through_gtest_output(cdt);
    rerun_gtest(cdt);
    schedule_gtest_task_with_filter(cdt);
    display_help(USR_CMD_DEFS, cdt);
    search_through_text_buffer(cdt);
    execute_restart_task(cdt);
    schedule_gtest_executions(cdt);
    schedule_task_executions(cdt);
    start_next_execution_with_debugger(cdt);
    start_next_execution(cdt);
    process_execution_event(cdt);
    parse_gtest_output(cdt);
    display_gtest_execution_result(cdt);
    stream_execution_output(cdt);
    stream_execution_output_on_failure(cdt);
    find_and_highlight_file_links(cdt);
    print_execution_output(cdt);
    display_execution_result(cdt);
    restart_repeating_gtest_on_success(cdt);
    restart_repeating_execution_on_success(cdt);
    finish_gtest_execution(cdt);
    finish_task_execution(cdt);
}
