#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <fstream>
#include <memory>
#include <numeric>
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
#include <unordered_map>
#include <unordered_set>
#include <utility>
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
const std::string TEMPLATE_ARG_PLACEHOLDER = "{}";
const std::string GTEST_TASK = "__gtest";

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

typedef uint32_t entity;

enum execution_event_type {
    execution_event_type_stdout, execution_event_type_stderr, execution_event_type_exit
};

struct execution_event
{
    entity exec;
    execution_event_type type;
    std::string data;
};

enum execution_state {
    execution_state_running, execution_state_complete, execution_state_failed
};

struct execution
{
    std::string name;
    std::string shell_command;
    execution_state state = execution_state_running;
    std::string stdout_line_buffer;
    std::string stderr_line_buffer;
    std::vector<std::string> output_lines;
};

struct gtest_test {
    std::string name;
    std::string duration;
    std::vector<std::string> output;
};

enum gtest_execution_state {
    gtest_execution_state_running, gtest_execution_state_parsing, gtest_execution_state_parsed, gtest_execution_state_finished
};

struct gtest_execution {
    bool rerun_of_single_test = false;
    std::vector<gtest_test> tests;
    std::vector<size_t> failed_test_ids;
    size_t test_count = 0;
    std::optional<size_t> current_test;
    std::string total_duration;
    gtest_execution_state state = gtest_execution_state_running;
};

struct execution_output
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

struct cdt {
    std::deque<entity> execs_to_run_in_order;
    std::unordered_map<entity, size_t> task_ids;
    std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>> processes;
    std::unordered_map<entity, execution> execs;
    std::unordered_map<entity, execution_output> exec_outputs;
    std::unordered_map<entity, gtest_execution> gtest_execs;
    std::unordered_set<entity> repeat_until_fail;
    std::unordered_set<entity> stream_output;
    moodycamel::BlockingConcurrentQueue<execution_event> exec_event_queue;
    user_command last_usr_cmd;
    std::vector<task> tasks;
    std::vector<std::vector<size_t>> pre_tasks;
    template_string open_in_editor_cmd;
    const char* cdt_executable;
    std::filesystem::path tasks_config_path;
    std::vector<std::string> config_errors;
    entity entity_seed = 1;
};

const entity LAST_ENTITY = 0;

std::vector<user_command_definition> USR_CMD_DEFS;
const auto TASK = push_back_and_return(USR_CMD_DEFS, {"t", "ind", "Execute the task with the specified index"});
const auto TASK_REPEAT = push_back_and_return(USR_CMD_DEFS, {"tr", "ind", "Keep executing the task with the specified index until it fails"});
const auto OPEN = push_back_and_return(USR_CMD_DEFS, {"o", "ind", "Open the file link with the specified index in your code editor"});
const auto GTEST = push_back_and_return(USR_CMD_DEFS, {"g", "ind", "Display output of the specified google test"});
const auto GTEST_RERUN = push_back_and_return(USR_CMD_DEFS, {"gt", "ind", "Re-run the google test with the specified index"});
const auto GTEST_RERUN_REPEAT = push_back_and_return(USR_CMD_DEFS, {"gtr", "ind", "Keep re-running the google test with the specified index until it fails"});
const auto HELP = push_back_and_return(USR_CMD_DEFS, {"h", "", "Display list of user commands"});

std::optional<TinyProcessLib::Process::id_type> current_execution_pid;

static entity create_entity(cdt& cdt) {
    const auto res = cdt.entity_seed++;
    if (res != LAST_ENTITY) {
        return res;
    } else {
        return cdt.entity_seed++;
    }
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

static void init_example_user_config() {
    if (!std::filesystem::exists(USER_CONFIG_PATH)) {
        std::ofstream file(USER_CONFIG_PATH);
        file << "{" << std::endl;
        file << "  // Open file links from the output in Sublime Text:" << std::endl;
        file << "  //\"" << OPEN_IN_EDITOR_COMMAND_PROPERTY << "\": \"subl {}\"" << std::endl;
        file << "  // Open file links from the output in VSCode:" << std::endl;
        file << "  //\"" << OPEN_IN_EDITOR_COMMAND_PROPERTY << "\": \"code {}\"" << std::endl;
        file << "}" << std::endl;
    }
}

static void read_user_config(cdt& cdt) {
    nlohmann::json config_json;
    if (!parse_json_file(USER_CONFIG_PATH, false, config_json, cdt.config_errors)) {
        return;
    }
    std::vector<std::string> config_errors;
    read_property(config_json, OPEN_IN_EDITOR_COMMAND_PROPERTY, cdt.open_in_editor_cmd.str, true, "", "must be a string in format: 'notepad++ {}', where {} will be replaced with a file name", config_errors, [&cdt] () {
        const auto pos = cdt.open_in_editor_cmd.str.find(TEMPLATE_ARG_PLACEHOLDER);
        if (pos == std::string::npos) {
            return false;
        } else {
            cdt.open_in_editor_cmd.arg_pos = pos;
            return true;
        }
    });
    append_config_errors(USER_CONFIG_PATH, config_errors, cdt.config_errors);
}

static void read_tasks_config(cdt& cdt) {
    nlohmann::json config_json;
    std::vector<std::string> config_errors;
    if (!parse_json_file(cdt.tasks_config_path, true, config_json, cdt.config_errors)) {
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
    std::cerr << TERM_COLOR_RED;
    for (const auto& e: cdt.config_errors) {
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

template<typename T>
static bool is_cmd_arg_in_range(const user_command& cmd, const std::vector<T>& range) {
    return cmd.arg > 0 && cmd.arg <= range.size();
}

static bool is_active_execution(const std::optional<execution>& exec) {
    return exec && exec->state == execution_state_running;
}

static bool is_finished_execution(const std::optional<execution>& exec) {
    return exec && exec->state != execution_state_running;
}

static void read_user_command_from_stdin(cdt& cdt) {
    if (!cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    std::cout << TERM_COLOR_GREEN << "(cdt) " << TERM_COLOR_RESET;
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        cdt.last_usr_cmd.executed = false;
    } else {
        cdt.last_usr_cmd = parse_user_command(input);
    }
}

static void read_user_command_from_env(cdt& cdt) {
    const auto str = getenv(ENV_VAR_LAST_COMMAND);
    if (str) {
        cdt.last_usr_cmd = parse_user_command(str);
    }
}

static void display_list_of_tasks(const std::vector<task>& tasks) {
    std::cout << TERM_COLOR_GREEN << "Tasks:" << TERM_COLOR_RESET << std::endl;
    for (auto i = 0; i < tasks.size(); i++) {
        std::cout << std::to_string(i + 1) << " \"" << tasks[i].name << "\"" << std::endl;
    }
}

static void schedule_task(cdt& cdt) {
    if (!accept_usr_cmd(TASK, cdt.last_usr_cmd) && !accept_usr_cmd(TASK_REPEAT, cdt.last_usr_cmd)) return;
    if (!is_cmd_arg_in_range(cdt.last_usr_cmd, cdt.tasks)) {
        display_list_of_tasks(cdt.tasks);
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
        cdt.execs_to_run_in_order.push_back(task_exec);
    }
}

static void terminate_current_execution_or_exit(int signal) {
    if (current_execution_pid) {
        TinyProcessLib::Process::kill(*current_execution_pid);
    } else {
        std::signal(signal, SIG_DFL);
        std::raise(signal);
    }
}

static void execute_restart_task(cdt& cdt) {
    if (cdt.execs_to_run_in_order.empty() || !cdt.processes.empty()) return;
    const auto exec = cdt.execs_to_run_in_order.front();
    if (cdt.tasks[cdt.task_ids[exec]].command != "__restart") return;
    destroy_entity(exec, cdt);
    std::cout << TERM_COLOR_MAGENTA << "Restarting program..." << TERM_COLOR_RESET << std::endl;
    const auto cmd_str = cdt.last_usr_cmd.cmd + std::to_string(cdt.last_usr_cmd.arg);
    setenv(ENV_VAR_LAST_COMMAND, cmd_str.c_str(), true);
    std::vector<const char*> argv = {cdt.cdt_executable, cdt.tasks_config_path.c_str(), nullptr};
    execvp(argv[0], const_cast<char* const*>(argv.data()));
    std::cout << TERM_COLOR_RED << "Failed to restart: " << std::strerror(errno) << TERM_COLOR_RESET << std::endl;
}

static void schedule_gtest_executions(cdt& cdt) {
    for (const auto exec: cdt.execs_to_run_in_order) {
        const auto& task = cdt.tasks[cdt.task_ids[exec]];
        if (task.command.find(GTEST_TASK) == 0) {
            if (!find(exec, cdt.execs)) {
                cdt.execs[exec] = execution{task.name, task.command.substr(GTEST_TASK.size() + 1)};
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
    cdt.processes[exec_entity] = std::make_unique<TinyProcessLib::Process>(
        exec.shell_command, "",
        write_to(cdt.exec_event_queue, exec_entity, execution_event_type_stdout),
        write_to(cdt.exec_event_queue, exec_entity, execution_event_type_stderr),
        handle_exit(cdt.exec_event_queue, exec_entity));
    current_execution_pid = cdt.processes[exec_entity]->get_id();
    cdt.exec_outputs[exec_entity] = execution_output{};
    if (find(exec_entity, cdt.stream_output)) {
        std::cout << TERM_COLOR_MAGENTA << "Running \"" << exec.name << "\"" << TERM_COLOR_RESET << std::endl;
    } else {
        std::cout << TERM_COLOR_BLUE << "Running \"" << exec.name << "\"..." << TERM_COLOR_RESET << std::endl;
    }
}

static void process_execution_event(cdt& cdt) {
    if (cdt.processes.empty()) return;
    execution_event event;
    cdt.exec_event_queue.wait_dequeue(event);
    auto& exec = cdt.execs[event.exec];
    if (event.type == execution_event_type_exit) {
        exec.state = cdt.processes[event.exec]->get_exit_status() == 0 ? execution_state_complete : execution_state_failed;
    } else {
        auto& line_buffer = event.type == execution_event_type_stdout ? exec.stdout_line_buffer : exec.stderr_line_buffer;
        auto to_process = line_buffer + event.data;
        std::stringstream tmp_buffer;
        for (const auto c: to_process) {
            if (c == '\n') {
                exec.output_lines.push_back(tmp_buffer.str());
                tmp_buffer = std::stringstream();
            } else {
                tmp_buffer << c;
            }
        }
        line_buffer = tmp_buffer.str();
    }
}

static void complete_current_gtest(gtest_execution& gtest_exec, const std::string& line_content) {
    gtest_exec.tests[*gtest_exec.current_test].duration = line_content.substr(line_content.rfind('('));
    gtest_exec.current_test.reset();
}

static void parse_gtest_output(cdt& cdt) {
    static const auto test_count_index = std::string("Running ").size();
    for (const auto& proc: cdt.processes) {
        auto& exec = cdt.execs[proc.first];
        auto& exec_output = cdt.exec_outputs[proc.first];
        const auto gtest_exec = find(proc.first, cdt.gtest_execs);
        const auto stream_output = find(proc.first, cdt.stream_output);
        if (gtest_exec && (gtest_exec->state == gtest_execution_state_running || gtest_exec->state == gtest_execution_state_parsing)) {
            for (const auto& line: exec.output_lines) {
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
                    complete_current_gtest(*gtest_exec, line_content);
                } else if (found_word == "FAILED" && line_content.find(gtest_exec->tests.back().name) == 0) {
                    gtest_exec->failed_test_ids.push_back(*gtest_exec->current_test);
                    complete_current_gtest(*gtest_exec, line_content);
                } else if (gtest_exec->current_test) {
                    gtest_exec->tests[*gtest_exec->current_test].output.push_back(line);
                    if (stream_output && gtest_exec->rerun_of_single_test) {
                        exec_output.output_lines.push_back(line);
                    }
                } else if (found_word == "RUN") {
                    gtest_exec->current_test = gtest_exec->tests.size();
                    gtest_exec->tests.push_back(gtest_test{line_content});
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
                if (stream_output && !gtest_exec->rerun_of_single_test && !gtest_exec->current_test) {
                    std::cout << std::flush << "\rTests completed: " << gtest_exec->tests.size() << " of " << gtest_exec->test_count;
                }
            }
            exec.output_lines.clear();
        }
    }
}

static void print_gtest_list(const std::vector<size_t>& test_ids, const std::vector<gtest_test>& tests) {
    for (auto i = 0; i < test_ids.size(); i++) {
        const auto id = test_ids[i];
        const auto& test = tests[id];
        std::cout << i + 1 << " \"" << test.name << "\" " << test.duration << std::endl;
    }
}

static void print_failed_gtest_list(const gtest_execution& exec) {
    std::cout << TERM_COLOR_RED << "Failed tests:" << TERM_COLOR_RESET << std::endl;
    print_gtest_list(exec.failed_test_ids, exec.tests);
    const int failed_percent = exec.failed_test_ids.size() / (float)exec.tests.size() * 100;
    std::cout << TERM_COLOR_RED << "Tests failed: " << exec.failed_test_ids.size() << " of " << exec.tests.size() << " (" << failed_percent << "%) " << exec.total_duration << TERM_COLOR_RESET << std::endl;
}

static void print_gtest_output(execution_output& out, const gtest_test& test, const std::string& color) {
    out = execution_output{};
    out.output_lines.reserve(test.output.size() + 1);
    out.output_lines.emplace_back(color + "\"" + test.name + "\" output:" + TERM_COLOR_RESET);
    out.output_lines.insert(out.output_lines.end(), test.output.begin(), test.output.end());
}

static gtest_test* find_gtest_by_cmd_arg(const user_command& cmd, gtest_execution* exec) {
    if (!exec || exec->test_count == 0) {
        std::cout << TERM_COLOR_GREEN << "No google tests have been executed yet." << TERM_COLOR_RESET << std::endl;
    } else if (exec->failed_test_ids.empty()) {
        if (!is_cmd_arg_in_range(cmd, exec->tests)) {
            std::cout << TERM_COLOR_GREEN << "Last executed tests " << exec->total_duration << ":" << TERM_COLOR_RESET << std::endl;
            std::vector<size_t> ids(exec->tests.size());
            std::iota(ids.begin(), ids.end(), 0);
            print_gtest_list(ids, exec->tests);
        } else {
            return &exec->tests[cmd.arg - 1];
        }
    } else {
        if (!is_cmd_arg_in_range(cmd, exec->failed_test_ids)) {
            print_failed_gtest_list(*exec);
        } else {
            return &exec->tests[exec->failed_test_ids[cmd.arg - 1]];
        }
    }
    return nullptr;
}

static void display_gtest_output(cdt& cdt) {
    if (!accept_usr_cmd(GTEST, cdt.last_usr_cmd)) return;
    const auto last_gtest_exec = find(LAST_ENTITY, cdt.gtest_execs);
    const auto last_exec_output = find(LAST_ENTITY, cdt.exec_outputs);
    const auto test = find_gtest_by_cmd_arg(cdt.last_usr_cmd, last_gtest_exec);
    if (last_gtest_exec && last_exec_output && test) {
        print_gtest_output(*last_exec_output, *test, last_gtest_exec->failed_test_ids.empty() ? TERM_COLOR_GREEN : TERM_COLOR_RED);
    }
}

static void rerun_gtest(cdt& cdt) {
    if (!accept_usr_cmd(GTEST_RERUN, cdt.last_usr_cmd) && !accept_usr_cmd(GTEST_RERUN_REPEAT, cdt.last_usr_cmd)) return;
    const auto last_gtest_exec = find(LAST_ENTITY, cdt.gtest_execs);
    const auto gtest_task_id = find(LAST_ENTITY, cdt.task_ids);
    const auto test = find_gtest_by_cmd_arg(cdt.last_usr_cmd, last_gtest_exec);
    if (last_gtest_exec && gtest_task_id && test) {
        for (const auto& pre_task_id: cdt.pre_tasks[*gtest_task_id]) {
            const auto pre_task_exec = create_entity(cdt);
            cdt.task_ids[pre_task_exec] = pre_task_id;
            cdt.execs_to_run_in_order.push_back(pre_task_exec);
        }
        const auto exec = create_entity(cdt);
        cdt.task_ids[exec] = *gtest_task_id;
        cdt.execs[exec] = execution{test->name, cdt.tasks[*gtest_task_id].command.substr(GTEST_TASK.size() + 1) + " --gtest_filter='" + test->name + "'"};
        cdt.gtest_execs[exec] = gtest_execution{true};
        cdt.stream_output.insert(exec);
        if (GTEST_RERUN_REPEAT.cmd == cdt.last_usr_cmd.cmd) {
            cdt.repeat_until_fail.insert(exec);
        }
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
            std::cout << "\33[2K\r"; // Current line has test execution progress displayed. We will start displaying our output on top of it.
            if (gtest_exec->state == gtest_execution_state_running) {
                exec.state = execution_state_failed;
                const auto& task = cdt.tasks[task_id];
                const auto gtest_binary_path = task.command.substr(GTEST_TASK.size() + 1);
                std::cout << TERM_COLOR_RED << "'" << gtest_binary_path << "' is not a google test executable" << TERM_COLOR_RESET << std::endl;
            } else if (gtest_exec->state == gtest_execution_state_parsing) {
                exec.state = execution_state_failed;
                std::cout << TERM_COLOR_RED << "Tests have finished prematurely" << TERM_COLOR_RESET << std::endl;
                // Tests might have crashed in the middle of some test. If so - consider test failed.
                // This is not always true however: tests might have crashed in between two test cases.
                if (gtest_exec->current_test) {
                    gtest_exec->failed_test_ids.push_back(*gtest_exec->current_test);
                    gtest_exec->current_test.reset();
                }
            } else if (gtest_exec->failed_test_ids.empty() && find(proc.first, cdt.stream_output)) {
                std::cout << TERM_COLOR_GREEN << "Successfully executed " << gtest_exec->tests.size() << " tests "  << gtest_exec->total_duration << TERM_COLOR_RESET << std::endl;
            }
            if (!gtest_exec->failed_test_ids.empty()) {
                print_failed_gtest_list(*gtest_exec);
                if (gtest_exec->failed_test_ids.size() == 1) {
                    const auto& test = gtest_exec->tests[gtest_exec->failed_test_ids[0]];
                    print_gtest_output(exec_output, test, TERM_COLOR_RED);
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
        }
    }
}

static void finish_rerun_gtest_execution(cdt& cdt) {
    std::unordered_set<entity> to_destroy;
    for (const auto& proc: cdt.processes) {
        const auto gtest_exec = find(proc.first, cdt.gtest_execs);
        if (gtest_exec && cdt.execs[proc.first].state != execution_state_running && gtest_exec->rerun_of_single_test) {
            to_destroy.insert(proc.first);
        }
    }
    destroy_components(to_destroy, cdt.gtest_execs);
}

static void stream_execution_output(cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        auto& exec = cdt.execs[proc.first];
        auto& exec_output = cdt.exec_outputs[proc.first];
        if (find(proc.first, cdt.stream_output)) {
            exec_output.output_lines.insert(exec_output.output_lines.end(), exec.output_lines.begin(), exec.output_lines.end());
            exec.output_lines.clear();
        }
    }
}

static void find_and_highlight_file_links(cdt& cdt) {
    static const std::regex UNIX_FILE_LINK_REGEX("^(\\/[^:]+:[0-9]+(:[0-9]+)?)");
    if (cdt.open_in_editor_cmd.str.empty()) return;
    for (auto& out: cdt.exec_outputs) {
        for (auto i = out.second.lines_processed; i < out.second.output_lines.size(); i++) {
            auto& line = out.second.output_lines[i];
            std::smatch link_match;
            if (std::regex_search(line, link_match, UNIX_FILE_LINK_REGEX)) {
                std::stringstream highlighted_line;
                out.second.file_links.push_back(link_match[1]);
                highlighted_line << TERM_COLOR_MAGENTA << '[' << OPEN.cmd << out.second.file_links.size() << "] " << link_match[1] << TERM_COLOR_RESET << link_match.suffix();
                line = highlighted_line.str();
            }
        }
    }
}

static void print_execution_output(cdt& cdt) {
    for (auto& out: cdt.exec_outputs) {
        for (auto i = out.second.lines_processed; i < out.second.output_lines.size(); i++) {
            std::cout << out.second.output_lines[i] << std::endl;
        }
        out.second.lines_processed = out.second.output_lines.size();
    }
}

static void stream_execution_output_on_failure(cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        auto& exec = cdt.execs[proc.first];
        auto& exec_output = cdt.exec_outputs[proc.first];
        if (exec.state == execution_state_failed && !find(proc.first, cdt.stream_output)) {
            exec_output.output_lines.insert(exec_output.output_lines.end(), exec.output_lines.begin(), exec.output_lines.end());
            exec.output_lines.clear();
        }
    }
}

static void display_execution_result(cdt& cdt) {
    for (auto& proc: cdt.processes) {
        const auto& exec = cdt.execs[proc.first];
        if (exec.state != execution_state_running) {
            const auto code = proc.second->get_exit_status();
            if (exec.state == execution_state_failed) {
                std::cout << TERM_COLOR_RED << "'" << exec.name << "' failed: return code: " << code << TERM_COLOR_RESET << std::endl;
            } else if (find(proc.first, cdt.stream_output)) {
                std::cout << TERM_COLOR_MAGENTA << "'" << exec.name << "' complete: return code: " << code << TERM_COLOR_RESET << std::endl;
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
            move_component(proc.first, LAST_ENTITY, cdt.task_ids);
            move_component(proc.first, LAST_ENTITY, cdt.exec_outputs);
            move_component(proc.first, LAST_ENTITY, cdt.gtest_execs);
            current_execution_pid.reset();
        }
    }
    for (const auto e: to_destroy) {
        destroy_entity(e, cdt);
    }
}

static void open_file_link(cdt& cdt) {
    if (!accept_usr_cmd(OPEN, cdt.last_usr_cmd)) return;
    const auto exec_output = find(LAST_ENTITY, cdt.exec_outputs);
    if (cdt.open_in_editor_cmd.str.empty()) {
        std::cout << TERM_COLOR_RED << '\'' << OPEN_IN_EDITOR_COMMAND_PROPERTY << "' is not specified in " << USER_CONFIG_PATH << TERM_COLOR_RESET << std::endl;
    } else if (!exec_output || exec_output->file_links.empty()) {
        std::cout << TERM_COLOR_GREEN << "No file links in the output" << TERM_COLOR_RESET << std::endl;
    } else if (exec_output) {
        if (is_cmd_arg_in_range(cdt.last_usr_cmd, exec_output->file_links)) {
            const auto& link = exec_output->file_links[cdt.last_usr_cmd.arg - 1];
            std::string open_cmd = cdt.open_in_editor_cmd.str;
            open_cmd.replace(cdt.open_in_editor_cmd.arg_pos, TEMPLATE_ARG_PLACEHOLDER.size(), link);
            TinyProcessLib::Process(open_cmd).get_exit_status();
        } else {
            std::cout << TERM_COLOR_GREEN << "Last execution output:" << TERM_COLOR_RESET << std::endl;
            for (const auto& line: exec_output->output_lines) {
                std::cout << line << std::endl;
            }
        }
    }
}

static void prompt_user_to_ask_for_help() {
    std::cout << "Type " << TERM_COLOR_GREEN << HELP.cmd << TERM_COLOR_RESET << " to see list of all the user commands." << std::endl;
}

static void display_help(const std::vector<user_command_definition>& defs, cdt& cdt) {
    if (cdt.last_usr_cmd.executed) return;
    cdt.last_usr_cmd.executed = true;
    std::cout << TERM_COLOR_GREEN << "User commands:" << TERM_COLOR_RESET << std::endl;
    for (const auto& def: defs) {
        std::cout << def.cmd;
        if (!def.arg.empty()) {
            std::cout << '<' << def.arg << '>';
        }
        std::cout << ((def.cmd.size() + def.arg.size()) < 6 ? "\t\t" : "\t") << def.desc << std::endl;
    }
}

int main(int argc, const char** argv) {
    cdt cdt;
    cdt.cdt_executable = argv[0];
    init_example_user_config();
    read_argv(argc, argv, cdt);
    read_user_config(cdt);
    if (print_errors(cdt)) return 1;
    read_tasks_config(cdt);
    if (print_errors(cdt)) return 1;
    read_user_command_from_env(cdt);
    prompt_user_to_ask_for_help();
    display_list_of_tasks(cdt.tasks);
    std::signal(SIGINT, terminate_current_execution_or_exit);
    while (true) {
        read_user_command_from_stdin(cdt);
        schedule_task(cdt);
        open_file_link(cdt);
        display_gtest_output(cdt);
        rerun_gtest(cdt);
        display_help(USR_CMD_DEFS, cdt);
        execute_restart_task(cdt);
        schedule_gtest_executions(cdt);
        schedule_task_executions(cdt);
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
        finish_rerun_gtest_execution(cdt);
        finish_task_execution(cdt);
    }
}
