#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <vector>

#include "cdt.h"
#include "common.h"
#include "config.h"
#include "json.hpp"

template <typename T>
static bool ReadProperty(const nlohmann::json& json, const std::string& name, T& value,
                         bool is_optional, const std::string& err_prefix, const std::string& err_suffix,
                         std::vector<std::string>& errors, std::function<bool()> validate_result = [] {return true;}) {
    auto it = json.find(name);
    std::string err_msg = err_prefix + "'" + name + "': " + err_suffix;
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

bool ReadArgv(int argc, const char** argv, Cdt& cdt) {
  if (argc < 2) {
    cdt.config_errors.emplace_back("usage: cpp-dev-tools tasks.json [profile]");
    return false;
  }
  cdt.tasks_config_path = cdt.os->AbsolutePath(argv[1]);
  if (argc > 2) {
   cdt.selected_config_profile = argv[2];
  }
  std::filesystem::path config_dir_path = cdt.tasks_config_path.parent_path();
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
  std::filesystem::path home(cdt.os->GetEnv(kEnvVarHome));
  cdt.user_config_path = home / ".cpp-dev-tools.json";
  if (!cdt.os->FileExists(cdt.user_config_path)) {
    std::stringstream example;
    example << "{\n";
    example << "  // Open file links from the output in Sublime Text:\n";
    example << "  //\"" << kOpenInEditorCommandProperty << "\": \"subl {}\"\n";
    example << "  // Open file links from the output in VSCode:\n";
    example << "  //\"" << kOpenInEditorCommandProperty << "\": \"code {}\"\n";
    example << "  // Debug tasks on MacOS:\n";
    std::string mac_osascript =
        "on run argv\\n"
        "tell application \\\"Terminal\\\" to activate\\n"
        "tell application \\\"System Events\\\""
        " to tell process \\\"Terminal\\\""
        " to keystroke \\\"t\\\" using command down\\n"
        "tell application \\\"Terminal\\\""
        " to do script(\\\"cd \\\" & item 1 of argv & \\\" &&"
        " lldb -- \\\" & item 2 of argv) in selected tab of the front window\\n"
        "end run";
    std::string osascript_file = "/tmp/cdt-dbg.scpt";
    example << "  //\"" << kDebugCommandProperty << "\": \"echo '"
            << mac_osascript << "' > " << osascript_file << " && osascript "
            << osascript_file << " '{current_dir}' '{shell_cmd}'\"\n";
    example << "}\n";
    cdt.os->WriteFile(cdt.user_config_path, example.str());
  }
}

static std::function<bool()> ValidateTemplateString(
        const std::string& temp_str, const std::string& arg_name) {
    return [&] () {
        return temp_str.find(arg_name) != std::string::npos;
    };
}

void ReadUserConfig(Cdt& cdt) {
    static const std::string kErrSuffix = "must be a string in format, examples of which you can find in the config";
    nlohmann::json config_json;
    if (!ParseJsonFile(cdt.user_config_path, false, config_json,
                       cdt.config_errors, cdt)) {
        return;
    }
    std::vector<std::string> config_errors;
    ReadProperty(config_json, kOpenInEditorCommandProperty,
                 cdt.open_in_editor_cmd, true, "", kErrSuffix, config_errors,
                 ValidateTemplateString(cdt.open_in_editor_cmd, "{}"));
    ReadProperty(config_json, kDebugCommandProperty, cdt.debug_cmd, true, "",
                 kErrSuffix, config_errors,
                 ValidateTemplateString(cdt.debug_cmd, "{shell_cmd}"));
    AppendConfigErrors(cdt.user_config_path, config_errors, cdt.config_errors);
}

void ReadTasksConfig(Cdt& cdt) {
  nlohmann::json config_json;
  std::vector<std::string> config_errors;
  if (!ParseJsonFile(cdt.tasks_config_path, true, config_json,
                     cdt.config_errors, cdt)) {
    return;
  }
  // Read profiles and find the selected one
  std::vector<nlohmann::json> profiles;
  int selected_profile = -1;
  if (ReadProperty(config_json, "cdt_profiles", profiles, true, "",
                   "must be an array of profile objects", config_errors)) {
    std::unordered_set<std::string> mandatory_variables = {"name"};
    // All profiles should have the same set of variables, otherwise switching
    // from one profile to another might break something because some variable
    // is not set.
    for (nlohmann::json& profile: profiles) {
      for (auto it = profile.begin(); it != profile.end(); it++) {
        mandatory_variables.insert(it.key());
      }
    }
    for (int i = 0; i < profiles.size(); i++) {
      std::string err_prefix = "profile #" + std::to_string(i + 1) + ": ";
      nlohmann::json& profile = profiles[i];
      std::optional<std::string> name;
      for (const std::string& var_name: mandatory_variables) {
        std::string var_value;
        ReadProperty(profile, var_name, var_value, false, err_prefix,
                     "must be a string", config_errors);
        if (var_name == "name") {
          name = var_value;
        }
      }
      if (!cdt.selected_config_profile) {
        cdt.selected_config_profile = name;
      }
      if (cdt.selected_config_profile == name) {
        selected_profile = i;
      }
    }
  }
  if (cdt.selected_config_profile && selected_profile == -1) {
    config_errors.push_back("profile with name '" +
                            *cdt.selected_config_profile +
                            "' is not defined in 'cdt_profiles'");
  }
  std::unordered_map<std::string, std::string> profile_variables;
  // Apply profile variables to tasks only if there are no errors.
  // If there are - selected_profile might be malformed.
  if (config_errors.empty() && selected_profile != -1) {
    profile_variables = profiles[selected_profile];
  }
  // Read tasks
  std::vector<nlohmann::json> tasks_json;
  if (!ReadProperty(config_json, "cdt_tasks", tasks_json, false, "",
                    "must be an array of task objects", config_errors)) {
    AppendConfigErrors(cdt.tasks_config_path, config_errors, cdt.config_errors);
    return;
  }
  // Initialize tasks with their direct "pre_tasks" dependencies
  std::vector<std::vector<std::string>> direct_pre_task_names(
      tasks_json.size());
  for (int i = 0; i < tasks_json.size(); i++) {
    Task new_task;
    nlohmann::json& task_json = tasks_json[i];
    std::string err_prefix = "task #" + std::to_string(i + 1) + ": ";
    ReadProperty(task_json, "name", new_task.name, false, err_prefix,
                 "must be a string", config_errors);
    ReadProperty(task_json, "command", new_task.command, false, err_prefix,
                 "must be a string", config_errors);
    ReadProperty(task_json, "pre_tasks", direct_pre_task_names[i], true,
                 err_prefix, "must be an array of other task names",
                 config_errors);
    // Apply profile variables to the task
    new_task.name = FormatTemplate(new_task.name, profile_variables);
    new_task.command = FormatTemplate(new_task.command, profile_variables);
    for (std::string& pre_task: direct_pre_task_names[i]) {
      pre_task = FormatTemplate(pre_task, profile_variables);
    }
    cdt.tasks.push_back(new_task);
  }
  std::vector<std::vector<size_t>> direct_pre_tasks(tasks_json.size());
  cdt.pre_tasks = std::vector<std::vector<size_t>>(tasks_json.size());
  for (int i = 0; i < direct_pre_task_names.size(); i++) {
    std::string err_prefix = "task #" + std::to_string(i + 1) + ": ";
    std::vector<std::string>& names = direct_pre_task_names[i];
    std::vector<size_t>& ids = direct_pre_tasks[i];
    for (std::string& name: names) {
      auto it = std::find_if(cdt.tasks.begin(), cdt.tasks.end(),
                             [&name] (Task& t) {return t.name == name;});
      if (it != cdt.tasks.end()) {
        ids.push_back(it - cdt.tasks.begin());
      } else {
        config_errors.push_back(err_prefix + "references task '" + name +
                                "' that does not exist");
      }
    }
  }
  // Validate that all task names are unique. Otherwise we will have problems
  // figuring out which of the tasks is the right pre task.
  for (int i = 0; i < cdt.tasks.size(); i++) {
    std::string& name = cdt.tasks[i].name;
    for (int j = 0; j < cdt.tasks.size(); j++) {
      if (i != j && name == cdt.tasks[j].name) {
        config_errors.push_back("task #" + std::to_string(i + 1) + ": name '" +
                                cdt.tasks[i].name + "' is already used by " +
                                "task #" + std::to_string(j + 1));
      }
    }
  }
  // Transform the "pre_tasks" dependency graph of each task into a flat vector
  // of effective pre_tasks.
  for (int i = 0; i < cdt.tasks.size(); i++) {
    std::string& primary_task_name = cdt.tasks[i].name;
    std::vector<size_t>& pre_tasks = cdt.pre_tasks[i];
    std::stack<size_t> to_visit;
    std::vector<size_t> task_call_stack;
    to_visit.push(i);
    task_call_stack.push_back(i);
    while (!to_visit.empty()) {
      size_t task_id = to_visit.top();
      std::vector<size_t>& pre_pre_tasks = direct_pre_tasks[task_id];
      // We are visiting each task with non-empty "pre_tasks" twice,
      // so when we get to a task the second time - the task should already be
      // on top of the task_call_stack.
      // If that's the case - don't push the task to stack second time.
      if (task_call_stack.back() != task_id) {
        task_call_stack.push_back(task_id);
      }
      bool all_children_visited = true;
      for (size_t child: pre_pre_tasks) {
        auto it = std::find(pre_tasks.begin(), pre_tasks.end(), child);
        if (it == pre_tasks.end()) {
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
        int task_in_call_stack_count = std::count(task_call_stack.begin(),
                                                  task_call_stack.end(),
                                                  task_id);
        if (task_in_call_stack_count > 1) {
          std::stringstream err;
          err << "task '" << primary_task_name
              << "' has a circular dependency in it's 'pre_tasks':\n";
          for (int j = 0; j < task_call_stack.size(); j++) {
            err << cdt.tasks[task_call_stack[j]].name;
            if (j + 1 < task_call_stack.size()) {
              err << " -> ";
            }
          }
          config_errors.emplace_back(err.str());
          break;
        }
        for (auto it = pre_pre_tasks.rbegin();
             it != pre_pre_tasks.rend();
             it++) {
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
    for (const std::string& e: cdt.config_errors) {
        cdt.os->Err() << e << std::endl;
    }
    cdt.os->Err() << kTcReset;
    return true;
}

void DisplayUsedConfigProfile(const Cdt &cdt) {
  if (cdt.selected_config_profile) {
    cdt.os->Out() << "Using profile " << kTcGreen
                  << *cdt.selected_config_profile << kTcReset << std::endl;
  }
}
