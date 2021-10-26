#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "process.hpp"
#include "json.hpp"
#include "concurrentqueue.h"

const auto TERM_COLOR_RED = "\033[31m";
const auto TERM_COLOR_GREEN = "\033[32m";
const auto TERM_COLOR_BLUE = "\033[34m";
const auto TERM_COLOR_MAGENTA = "\033[35m";
const auto TERM_COLOR_RESET = "\033[0m";

struct user_command
{
    std::string command;
    size_t arg = 0;
    bool executed = false;
};

struct task
{
    std::string name;
    std::string command;
    std::vector<std::string> pre_tasks;
};

static std::string error_invalid_config(const std::string& config) {
    return config + " is invalid:";
}

static void read_task_property(const nlohmann::json& task_json, size_t task_ind, const std::string& property,
                               std::string& output, std::stringstream& errors) {
    auto it = task_json.find(property);
    if (it == task_json.end()) {
        errors << "task #" + std::to_string(task_ind + 1) + " does not have '" + property + "' string specified\n";
    } else {
        output = *it;       
    }
}

static void read_task_property(const nlohmann::json& task_json, size_t task_ind, const std::string& property,
                               std::vector<std::string>& output, std::stringstream& errors, const std::string& err_desc) {
    auto it = task_json.find(property);
    if (it != task_json.end()) {
        try {
            output = it->get<std::vector<std::string>>();
        } catch (std::exception& e) {
            errors << "task #" + std::to_string(task_ind + 1) + " has '" << property << "' " << it->dump() << " which " << err_desc << "\n";
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
    for (auto i = 0; i < tasks_json.size(); i++) {
        task t;
        const auto& task_json = tasks_json[i];
        read_task_property(task_json, i, "name", t.name, errors_stream);
        read_task_property(task_json, i, "command", t.command, errors_stream);
        read_task_property(task_json, i, "pre_tasks", t.pre_tasks, errors_stream, "should be an array of other task names");
        tasks.push_back(t);
    }
    // Validate that tasks reference existing tasks
    for (const auto& t: tasks) {
        for (const auto& task_name: t.pre_tasks) {
            if (std::find_if(tasks.begin(), tasks.end(), [&task_name] (const task& ot) {return ot.name == task_name;}) == tasks.end()) {
                errors_stream << "task '" << t.name << "' references task '" << task_name << "' that does not exist\n";
            }
        }
    }
    const auto error = errors_stream.str();
    if (error.empty()) {
        return true;
    } else {
        std::cerr << error_invalid_config(config_path) << std::endl << error;
        return false;
    }
}

static void format_task_list_selection_msgs(const std::vector<task>& tasks, std::string& task_list_msg) {
    std::stringstream tl_msg_stream;
    tl_msg_stream << TERM_COLOR_GREEN << "Tasks:\n" << TERM_COLOR_RESET;
    for (auto i = 0; i < tasks.size(); i++) {
        tl_msg_stream << std::to_string(i + 1) << ") \"" << tasks[i].name << "\"\n";
    }
    task_list_msg = tl_msg_stream.str();
}

static void write_to_stdout(const char* data, size_t size) {
    std::cout << std::string(data, size);
}

static void write_to_stderr(const char* data, size_t size) {
    std::cerr << std::string(data, size);
}

static std::function<void(const char *bytes, size_t n)> write_to_buffer(moodycamel::ConcurrentQueue<char>& output) {
    return [&output] (const char* data, size_t size) {
        for (auto i = 0; i < size; i++) {
            output.enqueue(data[i]);
        }
    };
}

static bool execute_task_command(const task& t, bool is_primary_task, const char** argv) {
    if (is_primary_task) {
        std::cout << TERM_COLOR_MAGENTA << "Running \"" << t.name << "\"" << TERM_COLOR_RESET << std::endl;
    } else {
        std::cout << TERM_COLOR_BLUE << "Running \"" << t.name << "\"..." << TERM_COLOR_RESET << std::endl;
    }
    if (t.command == "__restart") {
        std::cout << TERM_COLOR_MAGENTA << "Restarting program..." << TERM_COLOR_RESET << std::endl;
        execvp(argv[0], const_cast<char* const*>(argv));
        std::cout << TERM_COLOR_RED << "Failed to restart: " << std::strerror(errno) << TERM_COLOR_RESET << std::endl;
        return false;
    } else {
        moodycamel::ConcurrentQueue<char> output;
        TinyProcessLib::Process process(t.command, "",
                                        is_primary_task ? write_to_stdout : write_to_buffer(output),
                                        is_primary_task ? write_to_stderr : write_to_buffer(output));
        const auto code = process.get_exit_status();
        const auto success = code == 0;
        std::cout.flush();
        std::cerr.flush();
        if (!success) {
            char c;
            while (output.try_dequeue(c)) {
                std::cerr << c;
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

static void read_user_command(user_command& cmd) {
    std::cout << TERM_COLOR_GREEN << "(cdt) " << TERM_COLOR_RESET;
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        return;
    }
    cmd = user_command{};
    std::stringstream chars;
    std::stringstream digits;
    for (auto i = 0; i < input.size(); i++) {
        auto c = input[i];
        if (std::isspace(c)) {
            continue;
        }
        if (std::isdigit(c)) {
            digits << c;
        } else {
            chars << c;
        }
    }
    cmd.command = chars.str();
    cmd.arg = std::atoi(digits.str().c_str());
}

static void display_list_of_tasks_on_unknown_cmd(user_command& cmd, const std::string& task_list) {
    if (cmd.executed) return;
    std::cout << task_list;
    cmd.executed = true;
}

static void execute_task(const std::vector<task>& tasks, user_command& cmd, const char** argv) {
    if (cmd.command != "t") return;
    cmd.executed = true;
    if (cmd.arg <= 0 || cmd.arg >= tasks.size()) {
        std::cerr << TERM_COLOR_RED << "There is no task with index " << cmd.arg << TERM_COLOR_RESET << std::endl;
        return;
    }
    const auto& to_execute = tasks[cmd.arg - 1];
    for (const auto& execute_before: to_execute.pre_tasks) {
        const auto it = std::find_if(tasks.begin(), tasks.end(), [&execute_before] (const task& t) {return t.name == execute_before;});
        if (!execute_task_command(*it, false, argv)) {
            return;
        }
    }
    execute_task_command(to_execute, true, argv);
}

int main(int argc, const char** argv) {
    std::vector<task> tasks;
    if (!read_tasks_from_specified_config_or_fail(argc, argv, tasks)) {
        return 1;
    }
    std::string task_list;
    format_task_list_selection_msgs(tasks, task_list);
    std::optional<task> task_to_execute;
    user_command cmd;
    display_list_of_tasks_on_unknown_cmd(cmd, task_list);
    while (true) {
        read_user_command(cmd);
        execute_task(tasks, cmd, argv);
        display_list_of_tasks_on_unknown_cmd(cmd, task_list);
    }
}