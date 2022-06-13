#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

using namespace testing;

class LaunchTest : public CdtTest {};

TEST_F(LaunchTest, StartAndViewTasks) {
  EXPECT_CALL(mock, Init());
  ASSERT_CDT_STARTED();
  std::string help_prompt = "\033[32mh\033[0m";
  EXPECT_OUT(HasSubstr(help_prompt));
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks_in_ui));
}

TEST_F(LaunchTest, FailToStartDueToUserConfigNotBeingJson) {
  mock.MockReadFile(paths.kUserConfig, "not a json");
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kUserConfig));
  EXPECT_OUT(HasSubstr("parse error"));
}

TEST_F(LaunchTest, FailToStartDueToUserConfigHavingPropertiesInIncorrectFormat) {
  nlohmann::json user_config_data;
  user_config_data["open_in_editor_command"] = "my-editor";
  user_config_data["debug_command"] = "my-debugger";
  mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasSubstr("'open_in_editor_command': must be a string in format"));
  EXPECT_OUT(HasSubstr("'debug_command': must be a string in format"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotSpecified) {
  std::vector<const char*> argv = {execs.kCdt.c_str()};
  EXPECT_FALSE(InitCdt(argv.size(), argv.data(), cdt));
  SaveOutput();
  EXPECT_OUT(HasSubstr("usage: cpp-dev-tools tasks.json [profile]"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotExisting) {
  mock.MockReadFile(paths.kTasksConfig);
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("does not exist"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotBeingJson) {
  mock.MockReadFile(paths.kTasksConfig, "not a json");
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("parse error"));
}

TEST_F(LaunchTest, FailToStartDueToCdtTasksNotBeingSpecifiedInConfig) {
  mock.MockReadFile(paths.kTasksConfig, "{}");
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("'cdt_tasks': must be an array of task objects"));
}

TEST_F(LaunchTest, FailToStartDueToCdtTasksNotBeingArrayOfObjects) {
  nlohmann::json tasks_config_data;
  tasks_config_data["cdt_tasks"] = "string";
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("'cdt_tasks': must be an array of task objects"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigHavingErrors) {
  nlohmann::json tasks_config_data;
  tasks_config_data["cdt_tasks"] = std::vector<nlohmann::json>{
    CreateTask(nullptr, "command"),
    CreateTask("name", nullptr, true),
    CreateTask("name 2", "command",
               std::vector<std::string>{"non-existent-task"}),
    CreateTask("cycle-1", "command", std::vector<std::string>{"cycle-2"}),
    CreateTask("cycle-2", "command", std::vector<std::string>{"cycle-3"}),
    CreateTask("cycle-3", "command", std::vector<std::string>{"cycle-1"}),
    CreateTask("duplicate name", "command"),
    CreateTask("duplicate name", "command"),
  };
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("task #1: 'name': must be a string"));
  EXPECT_OUT(HasSubstr("task #2: 'command': must be a string"));
  EXPECT_OUT(HasSubstr("task #2: 'pre_tasks': must be an array "
                       "of other task names"));
  EXPECT_OUT(HasSubstr("task #3: references task 'non-existent-task' "
                       "that does not exist"));
  EXPECT_OUT(HasSubstr("task #7: name 'duplicate name' is already used "
                       "by task #8"));
  EXPECT_OUT(HasSubstr("task #8: name 'duplicate name' is already used "
                       "by task #7"));
  EXPECT_OUT(HasSubstr("task 'cycle-1' has a circular dependency "
                       "in it's 'pre_tasks':\n"
                       "cycle-1 -> cycle-2 -> cycle-3 -> cycle-1"));
  EXPECT_OUT(HasSubstr("task 'cycle-2' has a circular dependency "
                       "in it's 'pre_tasks':\n"
                       "cycle-2 -> cycle-3 -> cycle-1 -> cycle-2"));
  EXPECT_OUT(HasSubstr("task 'cycle-3' has a circular dependency "
                       "in it's 'pre_tasks':\n"
                       "cycle-3 -> cycle-1 -> cycle-2 -> cycle-3"));
}

TEST_F(LaunchTest, StartAndChangeCwdToTasksConfigsDirectory) {
  EXPECT_CALL(mock, SetCurrentPath(paths.kTasksConfig.parent_path()));
  ASSERT_CDT_STARTED();
}

TEST_F(LaunchTest, StartAndCreateExampleUserConfig) {
  std::string default_user_config_content =
      "{\n"
      "  // Open file links from the output in Sublime Text:\n"
      "  //\"open_in_editor_command\": \"subl {}\"\n"
      "  // Open file links from the output in VSCode:\n"
      "  //\"open_in_editor_command\": \"code {}\"\n"
      "  // Debug tasks on MacOS:\n"
      "  //\"debug_command\": \"echo 'on run argv\\n"
      "tell application \\\"Terminal\\\" to activate\\n"
      "tell application \\\"System Events\\\" to tell process \\\"Terminal\\\" "
      "to keystroke \\\"t\\\" using command down\\n"
      "tell application \\\"Terminal\\\" to do script("
      "\\\"cd \\\" & item 1 of argv & \\\" && lldb -- \\\" & item 2 of argv) "
      "in selected tab of the front window\\nend run' > /tmp/cdt-dbg.scpt && "
      "osascript /tmp/cdt-dbg.scpt '{current_dir}' '{shell_cmd}'\"\n"
      "  // Debug tasks on Windows:\n"
      "  //\"debug_command\": \"devenv -DebugExe {shell_cmd}\"\n"
      "}\n";
  EXPECT_CALL(mock, FileExists(paths.kUserConfig))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock, WriteFile(paths.kUserConfig, default_user_config_content));
  ASSERT_CDT_STARTED();
}

TEST_F(LaunchTest, StartAndNotOverrideExistingUserConfig) {
  EXPECT_CALL(mock, WriteFile(Eq(paths.kUserConfig), _)).Times(0);
  ASSERT_CDT_STARTED();
}

TEST_F(LaunchTest, FailToStartDueToProfilesNotBeingArrayOfObjects) {
  tasks_config_data["cdt_profiles"] = "string";
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("'cdt_profiles': must be an array of profile objects"));
}

TEST_F(LaunchTest, FailedToStartDueToProfilesHavingErrors) {
  std::vector<nlohmann::json> profiles;
  // profile without a name and other variables
  profiles.push_back({});
  // profile with invalid variables
  profiles.push_back({
    {"name", "wrong profile"},
    {"a", 1},
    {"b", false},
    {"c", "string"},
    {"d", std::vector<nlohmann::json>()},
    {"e", {{"a", 1}, {"b", false}}},
  });
  // correct profile
  profiles.push_back({
    {"name", "correct profile"},
    {"a", "a"},
    {"b", "b"},
    {"c", "c"},
    {"d", "d"},
    {"e", "e"},
  });
  tasks_config_data["cdt_profiles"] = profiles;
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("profile #1: 'd': must be a string"));
  EXPECT_OUT(HasSubstr("profile #1: 'e': must be a string"));
  EXPECT_OUT(HasSubstr("profile #1: 'c': must be a string"));
  EXPECT_OUT(HasSubstr("profile #1: 'a': must be a string"));
  EXPECT_OUT(HasSubstr("profile #1: 'b': must be a string"));
  EXPECT_OUT(HasSubstr("profile #1: 'name': must be a string"));
  EXPECT_OUT(HasSubstr("profile #2: 'd': must be a string"));
  EXPECT_OUT(HasSubstr("profile #2: 'e': must be a string"));
  EXPECT_OUT(HasSubstr("profile #2: 'a': must be a string"));
  EXPECT_OUT(HasSubstr("profile #2: 'b': must be a string"));
}


TEST_F(LaunchTest, FailedToStartDueToNonExistentProfileBeingSelected) {
  EXPECT_CDT_FAILED_WITH_PROFILE("unknown profile");
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("profile with name 'unknown profile' is not defined "
                       "in 'cdt_profiles'"));
}

TEST_F(LaunchTest, StartWithFirstProfileAutoselected) {
  mock.MockReadFile(paths.kTasksConfig, tasks_config_with_profiles_data.dump());
  ASSERT_CDT_STARTED();
  EXPECT_OUT(HasSubstr("Using profile \033[32mprofile 1\033[0m"));
  EXPECT_OUT(HasSubstr("build for macos with profile profile 1"));
  EXPECT_OUT(HasSubstr("run on macos"));
}

TEST_F(LaunchTest, StartWithSpecifiedProfileSelected) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  EXPECT_OUT(HasSubstr("Using profile \033[32mprofile 2\033[0m"));
  EXPECT_OUT(HasSubstr("build for windows with profile profile 2"));
  EXPECT_OUT(HasSubstr("run on windows"));
}
