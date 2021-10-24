#include <iostream>
#include <fstream>
#include <sstream>
#include "process.hpp"
#include "json.hpp"

const auto TERM_COLOR_GREEN = "\033[32m";
const auto TERM_COLOR_MAGENTA = "\033[35m";
const auto TERM_COLOR_RESET = "\033[0m";

struct task
{
    std::string name;
    std::string command;
};

static std::string error_invalid_config(const std::string& config) {
    return config + " is invalid:";
}

static void read_task_property(const nlohmann::json& task_json, size_t task_ind, const std::string& property, std::string& output, std::stringstream& errors) {
    auto it = task_json.find(property);
    if (it == task_json.end()) {
        errors << "task #" + std::to_string(task_ind + 1) + " does not have '" + property + "' string specified\n";
    } else {
        output = *it;       
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
        tasks.push_back(t);
    }
    const auto error = errors_stream.str();
    if (error.empty()) {
        return true;
    } else {
        std::cerr << error_invalid_config(config_path) << std::endl << error;
        return false;
    }
}

static void format_task_list_selection_msgs(const std::vector<task>& tasks, std::string& task_list_msg, std::string& opts_msg) {
    std::stringstream tl_msg_stream;
    std::stringstream o_msg_stream;
    tl_msg_stream << TERM_COLOR_GREEN << "Tasks:\n" << TERM_COLOR_RESET;
    o_msg_stream << TERM_COLOR_GREEN << "Choose task to execute (";
    for (auto i = 0; i < tasks.size(); i++) {
        const auto& t = tasks[i];
        const auto option = std::to_string(i + 1);
        tl_msg_stream << option << ") \"" << t.name << "\"\n";
        o_msg_stream << option;
        if (i + 1 < tasks.size()) {
            o_msg_stream << '/';
        }
    }
    o_msg_stream << "): " << TERM_COLOR_RESET;
    task_list_msg = tl_msg_stream.str();
    opts_msg = o_msg_stream.str();
}

static void choose_task_to_execute(const std::vector<task>& tasks, const std::string& task_list_msg, const std::string& opts_msg,
                                   std::optional<task>& to_execute) {
    if (!to_execute) {
        std::cout << task_list_msg;
    }
    std::cout << opts_msg;
    std::string selected_task;
    std::getline(std::cin, selected_task);
    const auto index = std::atoi(selected_task.c_str());
    if (index > 0 && index <= tasks.size()) {
        to_execute = tasks[index - 1];
    } else {
        to_execute.reset();
    }
}

static void execute_task(std::optional<task>& to_execute) {
    if (to_execute) {
        std::cout << TERM_COLOR_MAGENTA << "Running" << TERM_COLOR_RESET << " \"" << to_execute->name << "\"" << std::endl;
        TinyProcessLib::Process process(to_execute->command);
        std::cout << TERM_COLOR_MAGENTA << "Return code:" << TERM_COLOR_RESET << ' ' << process.get_exit_status() << std::endl;
    }
}

int main(int argc, const char** argv) {
    std::vector<task> tasks;
    if (!read_tasks_from_specified_config_or_fail(argc, argv, tasks)) {
        return 1;
    }
    std::string task_list, selection_opts;
    format_task_list_selection_msgs(tasks, task_list, selection_opts);
    std::optional<task> task_to_execute;
    while (true) {
        choose_task_to_execute(tasks, task_list, selection_opts, task_to_execute);
        execute_task(task_to_execute);
    }
}