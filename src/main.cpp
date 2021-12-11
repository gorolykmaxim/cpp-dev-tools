#include <cstddef>
#include <iostream>
#include <fstream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <filesystem>
#include <stack>
#include <regex>
#include <set>
#include <queue>
#include <csignal>
#include <vector>
#include "process.hpp"
#include "json.hpp"
#include "blockingconcurrentqueue.h"

const auto TERM_COLOR_RED = "\033[31m";
const auto TERM_COLOR_GREEN = "\033[32m";
const auto TERM_COLOR_BLUE = "\033[34m";
const auto TERM_COLOR_MAGENTA = "\033[35m";
const auto TERM_COLOR_RESET = "\033[0m";

const auto USER_CONFIG_PATH = std::filesystem::path(getenv("HOME")) / ".cpp-dev-tools.json";
const auto OPEN_IN_EDITOR_COMMAND_PROPERTY = "open_in_editor_command";
const auto ENV_VAR_LAST_COMMAND = "LAST_COMMAND";
const std::regex UNIX_FILE_LINK_REGEX("^(\\/[^:]+:[0-9]+(:[0-9]+)?)");
const std::string TEMPLATE_ARG_PLACEHOLDER = "{}";

template <typename T>
T push_back_and_return(std::vector<T>& vec, T&& t) {
    vec.push_back(t);
    return t;
}

struct user_command_definition
{
    std::string cmd;
    std::string arg;
    std::string desc;
};

struct user_command
{
    std::string cmd;
    size_t arg = 0;
    bool executed = false;
};

struct task
{
    std::string name;
    std::string command;
};

enum task_event_type {
    task_event_type_stdout, task_event_type_stderr, task_event_type_exit
};

struct task_event
{
    task_event_type type;
    std::string data;
};

struct task_execution
{
    size_t task_id;
    bool is_primary_task = false;
    bool is_finished = false;
    std::unique_ptr<TinyProcessLib::Process> process;
    std::unique_ptr<moodycamel::BlockingConcurrentQueue<task_event>> event_queue = std::make_unique<moodycamel::BlockingConcurrentQueue<task_event>>();
    std::string stdout_line_buffer;
    std::string stderr_line_buffer;
    std::vector<std::string> output_lines;
};

struct task_output
{
    std::vector<std::string> output_lines;
    size_t lines_processed = 0;
    std::vector<std::string> file_links;
};

struct template_string
{
    std::string str;
    size_t arg_pos;
};

std::vector<user_command_definition> USR_CMD_DEFS;
const auto TASK = push_back_and_return(USR_CMD_DEFS, {"t", "ind", "Execute the task with the specified index"});
const auto OPEN = push_back_and_return(USR_CMD_DEFS, {"o", "ind", "Open the file link with the specified index in your code editor"});
const auto HELP = push_back_and_return(USR_CMD_DEFS, {"h", "", "Display list of user commands"});

// This is global, because it is accessed from signal handlers.
// It is only non-empty if it is actually running. If it is not running and has been set to is_finished=true - it will become empty shortly.
std::optional<task_execution> last_task_exec;

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

static bool read_argv(int argc, const char** argv, std::filesystem::path& tasks_config_path, std::vector<std::string>& errors) {
    if (argc < 2) {
        errors.emplace_back("usage: cpp-dev-tools tasks.json");
        return false;
    }
    tasks_config_path = std::filesystem::absolute(argv[1]);
    const auto& config_dir_path = tasks_config_path.parent_path();
    std::filesystem::current_path(config_dir_path);
    return true;
}

static bool parse_json_file(const std::filesystem::path& config_path, bool should_exist, nlohmann::json& config_json,
                            std::vector<std::string>& errors) {
    std::ifstream config_file(config_path);
    if (!config_file) {
        if (should_exist) {
            errors.push_back(config_path.string() + " does not exist");
        }
        return false;
    }
    config_file >> std::noskipws;
    const std::string config_data = std::string(std::istream_iterator<char>(config_file), std::istream_iterator<char>());
    try {
        config_json = nlohmann::json::parse(config_data);
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

static void read_user_config(template_string& open_in_editor_command, std::vector<std::string>& errors) {
    nlohmann::json config_json;
    if (!parse_json_file(USER_CONFIG_PATH, false, config_json, errors)) {
        return;
    }
    std::vector<std::string> config_errors;
    read_property(config_json, OPEN_IN_EDITOR_COMMAND_PROPERTY, open_in_editor_command.str, true, "", "must be a string in format: 'notepad++ {}', where {} will be replaced with a file name", config_errors, [&open_in_editor_command] () {
        const auto pos = open_in_editor_command.str.find(TEMPLATE_ARG_PLACEHOLDER);
        if (pos == std::string::npos) {
            return false;
        } else {
            open_in_editor_command.arg_pos = pos;
            return true;
        }
    });
    append_config_errors(USER_CONFIG_PATH, config_errors, errors);
}

static void read_tasks_config(const std::filesystem::path& config_path, std::vector<task>& tasks, std::vector<std::vector<size_t>>& effective_pre_tasks,
                              std::vector<std::string>& errors) {
    nlohmann::json config_json;
    std::vector<std::string> config_errors;
    if (!parse_json_file(config_path, true, config_json, errors)) {
        return;
    }
    std::vector<nlohmann::json> tasks_json;
    if (!read_property(config_json, "cdt_tasks", tasks_json, false, "", "must be an array of task objects", config_errors)) {
        append_config_errors(config_path, config_errors, errors);
        return;
    }
    std::vector<std::vector<size_t>> direct_pre_tasks(tasks_json.size());
    effective_pre_tasks = std::vector<std::vector<size_t>>(tasks_json.size());
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
        tasks.push_back(new_task);
    }
    // Transform the "pre_tasks" dependency graph of each task into a flat vector of effective pre_tasks.
    for (auto i = 0; i < tasks.size(); i++) {
        const auto& primary_task_name = tasks[i].name;
        auto& pre_tasks = effective_pre_tasks[i];
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
                        err << tasks[task_call_stack[j]].name;
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
    append_config_errors(config_path, config_errors, errors);
}

static bool print_errors(std::vector<std::string>& errors) {
    if (errors.empty()) return false;
    std::cerr << TERM_COLOR_RED;
    for (const auto& e: errors) {
        std::cerr << e << std::endl;
    }
    std::cerr << TERM_COLOR_RESET;
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

static bool is_active_execution(const std::optional<task_execution>& exec) {
    return exec && !exec->is_finished;
}

static void read_user_command_from_stdin(user_command& cmd, const std::queue<size_t>& tasks_to_exec,
                                         const std::optional<task_execution>& exec) {
    if (!tasks_to_exec.empty() || is_active_execution(exec)) return;
    std::cout << TERM_COLOR_GREEN << "(cdt) " << TERM_COLOR_RESET;
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        cmd.executed = false;
    } else {
        cmd = parse_user_command(input);
    }
}

static void read_user_command_from_env(user_command& cmd) {
    const auto str = getenv(ENV_VAR_LAST_COMMAND);
    if (str) {
        cmd = parse_user_command(str);
    }
}

static void display_list_of_tasks(const std::vector<task>& tasks) {
    std::cout << TERM_COLOR_GREEN << "Tasks:" << TERM_COLOR_RESET << std::endl;
    for (auto i = 0; i < tasks.size(); i++) {
        std::cout << std::to_string(i + 1) << " \"" << tasks[i].name << "\"" << std::endl;
    }
}

static void schedule_task(const std::vector<task>& tasks, const std::vector<std::vector<size_t>>& pre_tasks,
                          std::queue<size_t>& to_exec, user_command& cmd) {
    if (!accept_usr_cmd(TASK, cmd)) return;
    if (cmd.arg <= 0 || cmd.arg > tasks.size()) {
        display_list_of_tasks(tasks);
    } else {
        const auto id = cmd.arg - 1;
        for (const auto pre_task_id: pre_tasks[id]) {
            to_exec.push(pre_task_id);
        }
        to_exec.push(id);
    }
}

static void notify_task_execution_about_child_process_exit(int signal) {
    if (last_task_exec) {
        task_event exit_event{};
        exit_event.type = task_event_type_exit;
        last_task_exec->event_queue->enqueue(exit_event);
    }
}

static void terminate_active_task_or_exit(int signal) {
    if (is_active_execution(last_task_exec)) {
        last_task_exec->process->kill();
    } else {
        std::signal(signal, SIG_DFL);
        std::raise(signal);
    }
}

static void dequeue_task_to_execute(std::queue<size_t>& tasks_to_exec, std::optional<size_t>& task_to_exec, task_output& out,
                         const std::optional<task_execution>& exec, const std::vector<task>& tasks) {
    if (tasks_to_exec.empty() || is_active_execution(exec)) return;
    task_to_exec = tasks_to_exec.front();
    tasks_to_exec.pop();
    out = task_output{};
    const auto is_primary_task = tasks_to_exec.empty();
    const auto& task_name = tasks[*task_to_exec].name;
    if (is_primary_task) {
        std::cout << TERM_COLOR_MAGENTA << "Running \"" << task_name << "\"" << TERM_COLOR_RESET << std::endl;
    } else {
        std::cout << TERM_COLOR_BLUE << "Running \"" << task_name << "\"..." << TERM_COLOR_RESET << std::endl;
    }
}

static void execute_restart_task(std::optional<size_t>& to_exec, const std::vector<task>& tasks, const char* executable,
                                 const std::filesystem::path& tasks_config_path, const user_command& cmd) {
    if (!to_exec || tasks[*to_exec].command != "__restart") return;
    to_exec = std::optional<size_t>();
    std::cout << TERM_COLOR_MAGENTA << "Restarting program..." << TERM_COLOR_RESET << std::endl;
    const auto cmd_str = cmd.cmd + std::to_string(cmd.arg);
    setenv(ENV_VAR_LAST_COMMAND, cmd_str.c_str(), true);
    std::vector<const char*> argv = {executable, tasks_config_path.c_str(), nullptr};
    execvp(argv[0], const_cast<char* const*>(argv.data()));
    std::cout << TERM_COLOR_RED << "Failed to restart: " << std::strerror(errno) << TERM_COLOR_RESET << std::endl;
}

static std::function<void(const char*,size_t)> write_to(moodycamel::BlockingConcurrentQueue<task_event>& queue, task_event_type event_type) {
    return [&queue, event_type] (const char* data, size_t size) {
        task_event event;
        event.type = event_type;
        event.data = std::string(data, size);
        queue.enqueue(event);
    };
}

static void execute_task(std::optional<size_t>& to_exec, const std::vector<task>& tasks, std::optional<task_execution>& exec,
                         const std::queue<size_t>& tasks_to_exec) {
    if (!to_exec) return;
    exec = task_execution{*to_exec};
    exec->is_primary_task = tasks_to_exec.empty();
    exec->process = std::make_unique<TinyProcessLib::Process>(
        tasks[*to_exec].command,
        "",
        write_to(*exec->event_queue, task_event_type_stdout),
        write_to(*exec->event_queue, task_event_type_stderr));
    to_exec = std::optional<size_t>();
}

static void display_output_with_file_links(const task_output& out) {
    if (out.file_links.empty()) {
        std::cout << TERM_COLOR_GREEN << "No file links in the output" << TERM_COLOR_RESET << std::endl;
    } else {
        std::cout << TERM_COLOR_GREEN << "Last task output:" << TERM_COLOR_RESET << std::endl;
        for (const auto& line: out.output_lines) {
            std::cout << line << std::endl;
        }
    }
}

static void process_task_event(std::optional<task_execution>& exec) {
    if (!is_active_execution(exec)) return;
    task_event event;
    exec->event_queue->wait_dequeue(event);
    if (event.type == task_event_type_exit) {
        // We might get "exit" event before receiving last stdout/stderr event.
        // Re-queue the "exit" event back and process last stdout/stderr event first.
        if (exec->event_queue->size_approx() > 0) {
            const auto exit_event = event;
            // Read "out" event before queing "exit" back or we will hang!!!
            // In the opposite order, event_queue will read just-placed "exit" event
            // instead of "out" event.
            exec->event_queue->wait_dequeue(event);
            exec->event_queue->enqueue(exit_event);
        } else {
            exec->is_finished = true;
        }
    }
    if (event.type != task_event_type_exit) {
        auto& line_buffer = event.type == task_event_type_stdout ? exec->stdout_line_buffer : exec->stderr_line_buffer;
        auto to_process = line_buffer + event.data;
        std::stringstream tmp_buffer;
        for (const auto c: to_process) {
            if (c == '\n') {
                exec->output_lines.push_back(tmp_buffer.str());
                tmp_buffer = std::stringstream();
            } else {
                tmp_buffer << c;
            }
        }
        line_buffer = tmp_buffer.str();
    }
}

static void stream_primary_task_output(std::optional<task_execution>& exec, task_output& out) {
    if (!is_active_execution(exec) || !exec->is_primary_task) return;
    out.output_lines.insert(out.output_lines.end(), exec->output_lines.begin(), exec->output_lines.end());
    exec->output_lines.clear();
}

static void find_and_highlight_file_links(task_output& out, template_string& open_in_editor_cmd) {
    if (open_in_editor_cmd.str.empty()) return;
    for (auto i = out.lines_processed; i < out.output_lines.size(); i++) {
        auto& line = out.output_lines[i];
        std::smatch link_match;
        if (std::regex_search(line, link_match, UNIX_FILE_LINK_REGEX)) {
            std::stringstream highlighted_line;
            out.file_links.push_back(link_match[1]);
            highlighted_line << TERM_COLOR_MAGENTA << '[' << OPEN.cmd << out.file_links.size() << "] " << link_match[1] << TERM_COLOR_RESET << link_match.suffix();
            line = highlighted_line.str();
        }
    }
}

static void print_task_output(task_output& out) {
    for (auto i = out.lines_processed; i < out.output_lines.size(); i++) {
        std::cout << out.output_lines[i] << std::endl;
    }
    out.lines_processed = out.output_lines.size();
}

static void display_task_completion(task_output& out, const std::string& color, const std::string& task_name, const std::string& status,
                                    int code) {
    out.output_lines.push_back(color + '\'' + task_name + "' " + status + ": return code: " + std::to_string(code) + TERM_COLOR_RESET);
}

static void finish_task_execution(std::optional<task_execution>& exec, task_output& out, const std::vector<task>& tasks,
                                  std::queue<size_t>& tasks_to_exec) {
    if (!exec || !exec->is_finished) return;
    const auto code = exec->process->get_exit_status();
    const auto& task_name = tasks[exec->task_id].name;
    if (code != 0) {
        tasks_to_exec = {};
        if (!exec->is_primary_task) {
            out.output_lines.insert(out.output_lines.end(), exec->output_lines.begin(), exec->output_lines.end());
            exec->output_lines.clear();
        }
        display_task_completion(out, TERM_COLOR_RED, task_name, "failed", code);
    } else if (exec->is_primary_task) {
        display_task_completion(out, TERM_COLOR_MAGENTA, task_name, "complete", code);
    }
    exec = std::optional<task_execution>();
}

static void open_file_link(task_output& out, const template_string& open_in_editor_cmd, user_command& cmd) {
    if (!accept_usr_cmd(OPEN, cmd)) return;
    if (open_in_editor_cmd.str.empty()) {
        std::cout << TERM_COLOR_RED << '\'' << OPEN_IN_EDITOR_COMMAND_PROPERTY << "' is not specified in " << USER_CONFIG_PATH << TERM_COLOR_RESET << std::endl;
    } else if (cmd.arg <= 0 || cmd.arg > out.file_links.size()) {
        display_output_with_file_links(out);
    } else {
        const auto& link = out.file_links[cmd.arg - 1];
        std::string open_cmd = open_in_editor_cmd.str;
        open_cmd.replace(open_in_editor_cmd.arg_pos, TEMPLATE_ARG_PLACEHOLDER.size(), link);
        TinyProcessLib::Process(open_cmd).get_exit_status();
    }
}

static void prompt_user_to_ask_for_help() {
    std::cout << "Type " << TERM_COLOR_GREEN << HELP.cmd << TERM_COLOR_RESET << " to see list of all the user commands." << std::endl;
}

static void display_help(const std::vector<user_command_definition>& defs, user_command& cmd) {
    if (cmd.executed) return;
    cmd.executed = true;
    std::cout << TERM_COLOR_GREEN << "User commands:" << TERM_COLOR_RESET << std::endl;
    for (const auto& def: defs) {
        std::cout << def.cmd;
        if (!def.arg.empty()) {
            std::cout << '<' << def.arg << '>';
        }
        std::cout << "\t\t" << def.desc << std::endl;
    }
}

int main(int argc, const char** argv) {
    template_string open_in_editor_command;
    std::vector<task> tasks;
    std::vector<std::vector<size_t>> pre_tasks;
    std::queue<size_t> tasks_to_exec;
    std::optional<size_t> task_to_exec;
    user_command cmd;
    task_output last_task_output;
    auto executable = argv[0];
    std::filesystem::path tasks_config_path;
    std::vector<std::string> errors;
    read_argv(argc, argv, tasks_config_path, errors);
    read_user_config(open_in_editor_command, errors);
    if (print_errors(errors)) return 1;
    read_tasks_config(tasks_config_path, tasks, pre_tasks, errors);
    if (print_errors(errors)) return 1;
    read_user_command_from_env(cmd);
    prompt_user_to_ask_for_help();
    display_list_of_tasks(tasks);
    std::signal(SIGCHLD, notify_task_execution_about_child_process_exit);
    std::signal(SIGINT, terminate_active_task_or_exit);
    while (true) {
        read_user_command_from_stdin(cmd, tasks_to_exec, last_task_exec);
        schedule_task(tasks, pre_tasks, tasks_to_exec, cmd);
        open_file_link(last_task_output, open_in_editor_command, cmd);
        display_help(USR_CMD_DEFS, cmd);
        dequeue_task_to_execute(tasks_to_exec, task_to_exec, last_task_output, last_task_exec, tasks);
        execute_restart_task(task_to_exec, tasks, executable, tasks_config_path, cmd);
        execute_task(task_to_exec, tasks, last_task_exec, tasks_to_exec);
        process_task_event(last_task_exec);
        stream_primary_task_output(last_task_exec, last_task_output);
        finish_task_execution(last_task_exec, last_task_output, tasks, tasks_to_exec);
        find_and_highlight_file_links(last_task_output, open_in_editor_command);
        print_task_output(last_task_output);
    }
}
