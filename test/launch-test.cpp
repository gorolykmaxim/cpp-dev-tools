#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

using namespace testing;

class LaunchTest : public CdtTest {
protected:
  std::vector<nlohmann::json> example_profiles, example_tasks;

  void SetUp() override {
    example_profiles = {
        {{"name", "profile 1"}, {"os", "macos"}},
        {{"name", "profile 2"}, {"os", "windows"}}};
    example_tasks = {
        CreateTaskAndProcess("build for {os} with profile {name}"),
        CreateTaskAndProcess("run on {os}")};
    Init();
  }
};

TEST_F(LaunchTest, StartAndViewTasks) {
  EXPECT_CALL(mock, Init());
  tasks = {
      CreateTaskAndProcess("task 1"),
      CreateTaskAndProcess("task 2"),
      CreateTaskAndProcess("task 3")};
  MockTasksConfig();
  std::vector<std::string> task_names = {"1 \"task 1\"", "2 \"task 2\"",
                                         "3 \"task 3\""};
  ASSERT_INIT_CDT();
  std::string help_prompt = "\033[32mh\033[0m";
  EXPECT_OUT(HasSubstr(help_prompt));
  EXPECT_OUT(HasSubstrsInOrder(task_names));
}

TEST_F(LaunchTest, FailToStartDueToUserConfigNotBeingJson) {
  mock.MockReadFile(paths.kUserConfig, "not a json");
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kUserConfig));
  EXPECT_OUT(HasSubstr("parse error"));
}

TEST_F(LaunchTest, FailToStartDueToUserConfigHavingPropertiesInIncorrectFormat) {
  user_config_data["open_in_editor_command"] = "my-editor";
  user_config_data["debug_command"] = "my-debugger";
  mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasSubstr("'open_in_editor_command': must be a string in format"));
  EXPECT_OUT(HasSubstr("'debug_command': must be a string in format"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotSpecified) {
  args = {execs.kCdt.c_str()};
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasSubstr("usage: cpp-dev-tools tasks.json [profile]"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotExisting) {
  mock.MockReadFile(paths.kTasksConfig);
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("does not exist"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotBeingJson) {
  mock.MockReadFile(paths.kTasksConfig, "not a json");
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("parse error"));
}

TEST_F(LaunchTest, FailToStartDueToCdtTasksNotBeingSpecifiedInConfig) {
  mock.MockReadFile(paths.kTasksConfig, "{}");
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("'cdt_tasks': must be an array of task objects"));
}

TEST_F(LaunchTest, FailToStartDueToCdtTasksNotBeingArrayOfObjects) {
  nlohmann::json tasks_config_data;
  tasks_config_data["cdt_tasks"] = "string";
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("'cdt_tasks': must be an array of task objects"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigHavingErrors) {
  tasks = {CreateTask(nullptr, "command"), CreateTask("name", nullptr, true),
           CreateTask("name 2", "command", {"non-existent-task"}),
           CreateTask("cycle-1", "command", {"cycle-2"}),
           CreateTask("cycle-2", "command", {"cycle-3"}),
           CreateTask("cycle-3", "command", {"cycle-1"}),
           CreateTask("duplicate name", "command"),
           CreateTask("duplicate name", "command")};
  MockTasksConfig();
  EXPECT_INIT_CDT_FAILED();
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
  ASSERT_INIT_CDT();
}

TEST_F(LaunchTest, StartAndNotOverrideExistingUserConfig) {
  EXPECT_CALL(mock, WriteFile(Eq(paths.kUserConfig), _)).Times(0);
  ASSERT_INIT_CDT();
}

TEST_F(LaunchTest, FailToStartDueToProfilesNotBeingArrayOfObjects) {
  nlohmann::json tasks_config_data;
  tasks_config_data["cdt_profiles"] = "string";
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("'cdt_profiles': must be an array of profile objects"));
}

TEST_F(LaunchTest, FailedToStartDueToProfilesHavingErrors) {
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
  MockTasksConfig();
  EXPECT_INIT_CDT_FAILED();
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
  args.push_back("unknown profile");
  EXPECT_INIT_CDT_FAILED();
  EXPECT_OUT(HasPath(paths.kTasksConfig));
  EXPECT_OUT(HasSubstr("profile with name 'unknown profile' is not defined "
                       "in 'cdt_profiles'"));
}

TEST_F(LaunchTest, StartWithFirstProfileAutoselected) {
  profiles = example_profiles;
  tasks = example_tasks;
  MockTasksConfig();
  ASSERT_INIT_CDT();
  EXPECT_OUT(HasSubstr("Using profile \033[32mprofile 1\033[0m"));
  EXPECT_OUT(HasSubstr("build for macos with profile profile 1"));
  EXPECT_OUT(HasSubstr("run on macos"));
}

TEST_F(LaunchTest, StartWithSpecifiedProfileSelected) {
  profiles = example_profiles;
  tasks = example_tasks;
  args.push_back(profiles[1]["name"]);
  MockTasksConfig();
  ASSERT_INIT_CDT();
  EXPECT_OUT(HasSubstr("Using profile \033[32mprofile 2\033[0m"));
  EXPECT_OUT(HasSubstr("build for windows with profile profile 2"));
  EXPECT_OUT(HasSubstr("run on windows"));
}
