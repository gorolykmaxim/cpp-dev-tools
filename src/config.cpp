#include <sstream>

#include "config.h"
#include "json.hpp"

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

bool ReadArgv(int argc, const char** argv, Cdt& cdt) {
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

void InitExampleUserConfig(Cdt& cdt) {
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

void ReadUserConfig(Cdt& cdt) {
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

void ReadTasksConfig(Cdt& cdt) {
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

bool PrintErrors(const Cdt& cdt) {
    if (cdt.config_errors.empty()) return false;
    cdt.os->Err() << kTcRed;
    for (const auto& e: cdt.config_errors) {
        cdt.os->Err() << e << std::endl;
    }
    cdt.os->Err() << kTcReset;
    return true;
}
