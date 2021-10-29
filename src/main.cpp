#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <filesystem>
#include <stack>
#include <regex>
#include <set>
#include "process.hpp"
#include "json.hpp"

const auto TERM_COLOR_RED = "\033[31m";
const auto TERM_COLOR_GREEN = "\033[32m";
const auto TERM_COLOR_BLUE = "\033[34m";
const auto TERM_COLOR_MAGENTA = "\033[35m";
const auto TERM_COLOR_RESET = "\033[0m";

const auto ENV_VAR_LAST_COMMAND = "LAST_COMMAND";

const std::regex UNIX_FILE_LINK_REGEX("\n(\\/[^:]+:[0-9]+)");

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
    std::vector<size_t> pre_tasks;
};

struct task_output
{
    std::string output;
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

static bool accept_usr_cmd(const user_command_definition& def, user_command& cmd) {
    if (def.cmd == cmd.cmd) {
        cmd.executed = true;
        return true;
    } else {
        return false;
    }
}

static std::string error_invalid_config(const std::string& config) {
    return config + " is invalid:";
}

static void read_task_property(const nlohmann::json& task_json, size_t task_ind, const std::string& property,
                               std::string& output, std::stringstream& errors) {
    auto it = task_json.find(property);
    if (it == task_json.end()) {
        errors << "task #" + std::to_string(task_ind + 1) + " does not have '" + property + "' string specified\n";
    } else {
        try {
            output = it->get<std::string>();
        } catch (std::exception& e) {
            errors << "task #" << std::to_string(task_ind + 1) << " property '" + property + "' must be a string\n";
        }
    }
}

static void read_pre_tasks_of_task(const std::vector<nlohmann::json>& tasks_json, const nlohmann::json& task_json, size_t task_ind,
                                   std::vector<size_t>& pre_tasks, std::stringstream& errors) {
    const auto pre_task_names_json = task_json.find("pre_tasks");
    if (pre_task_names_json == task_json.end()) {
        return;
    }
    std::vector<std::string> pre_task_names;
    try {
        pre_task_names = pre_task_names_json->get<std::vector<std::string>>();
    } catch (std::exception& e) {
        errors << "task #" + std::to_string(task_ind + 1) + " has 'pre_tasks' " << pre_task_names_json->dump() << " which should be an array of other task names\n";
        return;
    }
    for (const auto& pre_task_name: pre_task_names) {
        auto pre_task_ind = -1;
        for (auto i = 0; i < tasks_json.size(); i++) {
            if (tasks_json[i]["name"] == pre_task_name) {
                pre_task_ind = i;
                break;
            }
        }
        if (pre_task_ind >= 0) {
            pre_tasks.push_back(pre_task_ind);
        } else {
            errors << "task #" << std::to_string(task_ind + 1) << " references task '" << pre_task_name << "' that does not exist\n";
        }
    }
}

static bool read_tasks_from_specified_config_or_fail(int argc, const char** argv, std::vector<task>& tasks) {
    if (argc < 2) {
        std::cerr << "usage: cpp-dev-tools tasks.json" << std::endl;
        return false;
    }
    const auto* config_path = argv[1];
    std::ifstream config_file(config_path);
    if (!config_file) {
        std::cerr << config_path << " does not exist" << std::endl;
        return false;
    }
    const auto config_dir = std::filesystem::absolute(std::filesystem::path(config_path)).parent_path();
    std::filesystem::current_path(config_dir);
    config_file >> std::noskipws;
    const std::string config_data = std::string(std::istream_iterator<char>(config_file), std::istream_iterator<char>());
    nlohmann::json config_json;
    try {
        config_json = nlohmann::json::parse(config_data);
    } catch (std::exception& e) {
        std::cerr << "Failed to parse " << config_path << ": " << e.what() << std::endl;
        return false;
    }
    const auto it = config_json.find("cdt_tasks");
    if (it == config_json.end()) {
        std::cerr << error_invalid_config(config_path) << " array of task objects 'cdt_tasks' is not defined" << std::endl;
        return false;
    }
    const auto tasks_json = it->get<std::vector<nlohmann::json>>();
    std::stringstream errors_stream;
    // Initialize tasks with their direct "pre_tasks" dependencies
    for (auto i = 0; i < tasks_json.size(); i++) {
        task t;
        const auto& task_json = tasks_json[i];
        read_task_property(task_json, i, "name", t.name, errors_stream);
        read_task_property(task_json, i, "command", t.command, errors_stream);
        read_pre_tasks_of_task(tasks_json, task_json, i, t.pre_tasks, errors_stream);
        tasks.push_back(t);
    }
    // Transform the "pre_tasks" dependency graph of each task into a flat vector of effective pre_tasks.
    std::vector<std::vector<size_t>> effective_pre_tasks(tasks.size());
    for (auto i = 0; i < tasks.size(); i++) {
        const auto& primary_task_name = tasks[i].name;
        auto& pre_tasks = effective_pre_tasks[i];
        std::stack<size_t> to_visit;
        std::vector<size_t> task_call_stack;
        to_visit.push(i);
        task_call_stack.push_back(i);
        while (!to_visit.empty()) {
            const auto task_id = to_visit.top();
            const auto& t = tasks[task_id];
            // We are visiting each task with non-empty "pre_tasks" twice, so when we get to a task the second time - the
            // task should already be on top of the task_call_stack. If that's the case - don't push the task to stack
            // second time.
            if (task_call_stack.back() != task_id) {
                task_call_stack.push_back(task_id);
            }
            auto all_children_visited = true;
            for (const auto& child: t.pre_tasks) {
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
                    errors_stream << "task '" << primary_task_name << "' has a circular dependency in it's 'pre_tasks':\n";
                    for (auto j = 0; j < task_call_stack.size(); j++) {
                        errors_stream << tasks[task_call_stack[j]].name;
                        if (j + 1 == task_call_stack.size()) {
                            errors_stream << '\n';
                        } else {
                            errors_stream << " -> ";
                        }
                    }
                    break;
                }
                for (auto it = t.pre_tasks.rbegin(); it != t.pre_tasks.rend(); it++) {
                    to_visit.push(*it);
                }
            }
        }
    }
    for (auto i = 0; i< tasks.size(); i++) {
        tasks[i].pre_tasks = effective_pre_tasks[i];
    }
    const auto error = errors_stream.str();
    if (error.empty()) {
        return true;
    } else {
        std::cerr << error_invalid_config(config_path) << std::endl << error;
        return false;
    }
}

static bool read_user_config(template_string& open_in_editor_command) {
    const auto config_path = std::filesystem::path(getenv("HOME")) / ".cpp-dev-tools.json";
    std::ifstream config_file(config_path);
    if (!config_file) {
        return true;
    }
    config_file >> std::noskipws;
    const std::string config_data = std::string(std::istream_iterator<char>(config_file), std::istream_iterator<char>());
    nlohmann::json config_json;
    try {
        config_json = nlohmann::json::parse(config_data);
    } catch (std::exception& e) {
        std::cerr << "Failed to parse " << config_path << ": " << e.what() << std::endl;
        return false;
    }
    auto prop = config_json.find("open_in_editor_command");
    if (prop != config_json.end()) {
        try {
            open_in_editor_command.str = prop->get<std::string>();
            const auto pos = open_in_editor_command.str.find(TEMPLATE_ARG_PLACEHOLDER);
            if (pos == std::string::npos) {
                throw std::exception();
            } else {
                open_in_editor_command.arg_pos = pos;
            }
        } catch (std::exception& e) {
            std::cerr << error_invalid_config(config_path) << " 'open_in_editor_command' must be a string in format: 'notepad++ {}', where {} will be replaced with a file name" << std::endl;
            return false;
        }

    }
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

static void read_user_command_from_stdin(user_command& cmd) {
    std::cout << TERM_COLOR_GREEN << "(cdt) " << TERM_COLOR_RESET;
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        return;
    }
    cmd = parse_user_command(input);
}

static void read_user_command_from_env(user_command& cmd) {
    const auto str = getenv(ENV_VAR_LAST_COMMAND);
    if (str) {
        cmd = parse_user_command(str);
    }
}

static std::function<void(const char*,size_t)> duplicate_to(std::ostream& stream, std::stringstream& buffer, std::mutex& mutex, bool write_to_stream) {
    return [&stream, &buffer, &mutex, write_to_stream] (const char* bytes, size_t n) {
        std::string str(bytes, n);
        if (write_to_stream) {
            stream << str;
        }
        std::lock_guard<std::mutex> lg(mutex);
        buffer << str;
    };
}

static bool execute_task_command(const task& t, bool is_primary_task, const char** argv, const user_command& cmd, task_output& failed_output) {
    failed_output = {};
    if (is_primary_task) {
        std::cout << TERM_COLOR_MAGENTA << "Running \"" << t.name << "\"" << TERM_COLOR_RESET << std::endl;
    } else {
        std::cout << TERM_COLOR_BLUE << "Running \"" << t.name << "\"..." << TERM_COLOR_RESET << std::endl;
    }
    if (t.command == "__restart") {
        std::cout << TERM_COLOR_MAGENTA << "Restarting program..." << TERM_COLOR_RESET << std::endl;
        const auto cmd_str = cmd.cmd + std::to_string(cmd.arg);
        setenv(ENV_VAR_LAST_COMMAND, cmd_str.c_str(), true);
        execvp(argv[0], const_cast<char* const*>(argv));
        std::cout << TERM_COLOR_RED << "Failed to restart: " << std::strerror(errno) << TERM_COLOR_RESET << std::endl;
        return false;
    } else {
        std::mutex mutex;
        std::stringstream buffer;
        TinyProcessLib::Process process(t.command, "",
                                        duplicate_to(std::cout, buffer, mutex, is_primary_task),
                                        duplicate_to(std::cerr, buffer, mutex, is_primary_task));
        const auto code = process.get_exit_status();
        const auto success = code == 0;
        std::cout.flush();
        std::cerr.flush();
        if (!success) {
            failed_output.output = buffer.str();
            if (!is_primary_task) {
                std::cerr << failed_output.output;
            }
        }
        if (is_primary_task || !success) {
            if (success) {
                std::cout << TERM_COLOR_MAGENTA << "'" << t.name << "' complete: ";
            } else {
                std::cout << TERM_COLOR_RED << "'" << t.name << "' failed: ";
            }
            std::cout << "return code: " << code << TERM_COLOR_RESET << std::endl;
        }
        return success;
    }
}

static void display_list_of_tasks(const std::vector<task>& tasks) {
    std::cout << TERM_COLOR_GREEN << "Tasks:" << TERM_COLOR_RESET << std::endl;
    for (auto i = 0; i < tasks.size(); i++) {
        std::cout << std::to_string(i + 1) << " \"" << tasks[i].name << "\"" << std::endl;
    }
}

static void execute_task(const std::vector<task>& tasks, user_command& cmd, const char** argv, task_output& failed_output) {
    if (!accept_usr_cmd(TASK, cmd)) return;
    if (cmd.arg <= 0 || cmd.arg > tasks.size()) {
        display_list_of_tasks(tasks);
        return;
    }
    const auto& to_execute = tasks[cmd.arg - 1];
    for (const auto& task_index: to_execute.pre_tasks) {
        if (!execute_task_command(tasks[task_index], false, argv, cmd, failed_output)) {
            return;
        }
    }
    execute_task_command(to_execute, true, argv, cmd, failed_output);
}

static void display_file_links(const task_output& out) {
    if (out.file_links.empty()) {
        std::cout << TERM_COLOR_GREEN << "No file links in the output" << TERM_COLOR_RESET << std::endl;
    } else {
        std::cout << TERM_COLOR_GREEN << "File links from the output:" << TERM_COLOR_RESET << std::endl;
    }
    for (auto i = 0; i < out.file_links.size(); i++) {
        std::cout << i + 1 << ' ' << out.file_links[i] << std::endl;
    }
}

static void display_file_links_from_task_output(task_output& out, const template_string& open_in_editor_cmd) {
    if (open_in_editor_cmd.str.empty() || out.output.empty() || !out.file_links.empty()) return;
    std::sregex_iterator start(out.output.begin(), out.output.end(), UNIX_FILE_LINK_REGEX);
    std::sregex_iterator end;
    std::set<std::string> links;
    for (auto it = start; it != end; it++) {
        links.insert((*it)[1].str());
    }
    out.file_links = std::vector<std::string>(links.begin(), links.end());
    display_file_links(out);
}

static void open_file_link(task_output& out, const template_string& open_in_editor_cmd, user_command& cmd) {
    if (!accept_usr_cmd(OPEN, cmd)) return;
    if (cmd.arg <= 0 || cmd.arg > out.file_links.size()) {
        display_file_links(out);
        return;
    }
    const auto& link = out.file_links[cmd.arg - 1];
    std::string open_cmd = open_in_editor_cmd.str;
    open_cmd.replace(open_in_editor_cmd.arg_pos, TEMPLATE_ARG_PLACEHOLDER.size(), link);
    TinyProcessLib::Process p(open_cmd);
}

static void prompt_user_to_ask_for_help() {
    std::cout << "Type " << TERM_COLOR_GREEN << HELP.cmd << TERM_COLOR_RESET << " to see list of all the user commands." << std::endl;
}

static void display_help(const std::vector<user_command_definition>& defs, user_command& cmd) {
    if (cmd.executed) return;
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
    user_command cmd;
    task_output last_failed_task_output;
    if (!read_user_config(open_in_editor_command) || !read_tasks_from_specified_config_or_fail(argc, argv, tasks)) {
        return 1;
    }
    read_user_command_from_env(cmd);
    prompt_user_to_ask_for_help();
    display_list_of_tasks(tasks);
    while (true) {
        read_user_command_from_stdin(cmd);
        execute_task(tasks, cmd, argv, last_failed_task_output);
        display_file_links_from_task_output(last_failed_task_output, open_in_editor_command);
        open_file_link(last_failed_task_output, open_in_editor_command, cmd);
        display_help(USR_CMD_DEFS, cmd);
    }
}