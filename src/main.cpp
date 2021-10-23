#include <iostream>
#include <fstream>
#include <sstream>
#include "process.hpp"
#include "json.hpp"

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

static void choose_task_to_execute(const std::vector<task>& tasks, std::optional<task>& to_execute) {
    std::cout << "Tasks:" << std::endl;
    std::stringstream options;
    options << "(";
    for (auto i = 0; i < tasks.size(); i++) {
        const auto& t = tasks[i];
        std::cout << i + 1 << ") \"" << t.name << "\"" << std::endl;
        options << i + 1;
        if (i + 1 < tasks.size()) {
            options << "/";
        }
    }
    options << ")";
    std::cout << "Choose task to execute " << options.str() << ": ";
    std::string selected_task;
    std::getline(std::cin, selected_task);
    const auto index = std::atoi(selected_task.c_str());
    if (index <= tasks.size()) {
        to_execute = tasks[index - 1];
    }
}

static void execute_task(std::optional<task>& to_execute) {
    if (to_execute) {
        std::cout << std::endl << "Running: \"" << to_execute->name << "\"" << std::endl;
        TinyProcessLib::Process process(to_execute->command);
        std::cout << "Return code: " << process.get_exit_status() << std::endl << std::endl;
    }
}

int main(int argc, const char** argv) {
    std::vector<task> tasks;
    if (!read_tasks_from_specified_config_or_fail(argc, argv, tasks)) {
        return 1;
    }
    while (true) {
        std::optional<task> task_to_execute;
        choose_task_to_execute(tasks, task_to_execute);
        execute_task(task_to_execute);
    }
}