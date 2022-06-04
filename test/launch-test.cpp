#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

class LaunchTest : public CdtTest {};

TEST_F(LaunchTest, StartAndViewTasks) {
  EXPECT_CALL(mock, Init());
  EXPECT_TRUE(InitTestCdt());
  std::vector<std::string> expected_tasks;
  nlohmann::json& tasks = tasks_config_data["cdt_tasks"];
  for (int i = 0; i < tasks.size(); i++) {
    std::string index = std::to_string(i + 1);
    std::string name = tasks[i]["name"].get<std::string>();
    expected_tasks.push_back(index + " \"" + name + '"');
  }
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr("\033[32mh\033[0m"))
      << "help prompt is displayed";
  EXPECT_THAT(actual, HasSubstrsInOrder(expected_tasks))
      << "list of tasks displayed";
}

TEST_F(LaunchTest, FailToStartDueToUserConfigNotBeingJson) {
  mock.MockReadFile(paths.kUserConfig, "not a json");
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kUserConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr("parse error"))
      << "caused by failure to parse json";
}

TEST_F(LaunchTest, FailToStartDueToUserConfigHavingPropertiesInIncorrectFormat) {
  nlohmann::json user_config_data;
  user_config_data["open_in_editor_command"] = "my-editor";
  user_config_data["debug_command"] = "my-debugger";
  mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr("'open_in_editor_command': "
                                         "must be a string in format"));
  EXPECT_THAT(actual, testing::HasSubstr("'debug_command': "
                                         "must be a string in format"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotSpecified) {
  std::vector<const char*> argv = {execs.kCdt.c_str()};
  EXPECT_FALSE(InitCdt(argv.size(), argv.data(), cdt));
  EXPECT_THAT(out.str(),
              testing::HasSubstr("usage: cpp-dev-tools tasks.json [profile]"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotExisting) {
  mock.MockReadFile(paths.kTasksConfig);
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kTasksConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr("does not exist"));
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotBeingJson) {
  mock.MockReadFile(paths.kTasksConfig, "not a json");
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kTasksConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr("parse error"))
      << "caused by failure to parse json";
}

TEST_F(LaunchTest, FailToStartDueToCdtTasksNotBeingSpecifiedInConfig) {
  mock.MockReadFile(paths.kTasksConfig, "{}");
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kTasksConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr(
      "'cdt_tasks': must be an array of task objects"));
}

TEST_F(LaunchTest, FailToStartDueToCdtTasksNotBeingArrayOfObjects) {
  nlohmann::json tasks_config_data;
  tasks_config_data["cdt_tasks"] = "string";
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kTasksConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr(
      "'cdt_tasks': must be an array of task objects"));
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
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kTasksConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr("task #1: 'name': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr(
      "task #2: 'command': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr(
      "task #2: 'pre_tasks': must be an array of other task names"));
  EXPECT_THAT(actual, testing::HasSubstr(
      "task #3: references task 'non-existent-task' that does not exist"));
  EXPECT_THAT(actual, testing::HasSubstr(
      "task #7: name 'duplicate name' is already used by task #8"));
  EXPECT_THAT(actual, testing::HasSubstr(
      "task #8: name 'duplicate name' is already used by task #7"));
  EXPECT_THAT(actual, testing::HasSubstr(
      "'cycle-1' has a circular dependency"));
  EXPECT_THAT(actual, HasSubstrsInOrder(std::vector<std::string>{
      "task 'cycle-1' has a circular dependency in it's 'pre_tasks':",
      "cycle-1 -> cycle-2 -> cycle-3 -> cycle-1",
      "task 'cycle-2' has a circular dependency in it's 'pre_tasks':",
      "cycle-2 -> cycle-3 -> cycle-1 -> cycle-2",
      "task 'cycle-3' has a circular dependency in it's 'pre_tasks':",
      "cycle-3 -> cycle-1 -> cycle-2 -> cycle-3",
  }));
}

TEST_F(LaunchTest, StartAndChangeCwdToTasksConfigsDirectory) {
  EXPECT_CALL(mock, SetCurrentPath(paths.kTasksConfig.parent_path()));
  EXPECT_TRUE(InitTestCdt());
}

TEST_F(LaunchTest, StartAndCreateExampleUserConfig) {
  std::string default_user_config_content =
      "{\n"
      "  // Open file links from the output in Sublime Text:\n"
      "  //\"open_in_editor_command\": \"subl {}\"\n"
      "  // Open file links from the output in VSCode:\n"
      "  //\"open_in_editor_command\": \"code {}\"\n"
      "  // Debug tasks on MacOS:\n"
      "  //\"debug_command\": \"osascript"
      " -e 'tell application \\\"Terminal\\\" to activate'"
      " -e 'tell application \\\"System Events\\\" to tell process"
      " \\\"Terminal\\\" to keystroke \\\"t\\\" using command down'"
      " -e 'tell application \\\"Terminal\\\" to do script"
      " \\\"cd {current_dir} && lldb -- {shell_cmd}\\\""
      " in selected tab of the front window'\"\n"
      "}\n";
  EXPECT_CALL(mock, FileExists(paths.kUserConfig))
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(mock, WriteFile(paths.kUserConfig, default_user_config_content));
  EXPECT_TRUE(InitTestCdt());
}

TEST_F(LaunchTest, StartAndNotOverrideExistingUserConfig) {
  EXPECT_CALL(mock, WriteFile(testing::Eq(paths.kUserConfig), testing::_))
      .Times(0);
  EXPECT_TRUE(InitTestCdt());
}

TEST_F(LaunchTest, FailToStartDueToProfilesNotBeingArrayOfObjects) {
  tasks_config_data["cdt_profiles"] = "string";
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kTasksConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr(
      "'cdt_profiles': must be an array of profile objects"));
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
  EXPECT_FALSE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kTasksConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr("profile #1: 'd': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #1: 'e': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #1: 'c': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #1: 'a': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #1: 'b': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #1: 'name': "
                                         "must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #2: 'd': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #2: 'e': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #2: 'a': must be a string"));
  EXPECT_THAT(actual, testing::HasSubstr("profile #2: 'b': must be a string"));
}


TEST_F(LaunchTest, FailedToStartDueToNonExistentProfileBeingSelected) {
  mock.MockReadFile(paths.kTasksConfig, tasks_config_with_profiles_data.dump());
  EXPECT_FALSE(InitTestCdt("unknown profile"));
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(paths.kTasksConfig))
      << "path to config mentioned";
  EXPECT_THAT(actual, testing::HasSubstr(
      "profile with name 'unknown profile' is not defined in 'cdt_profiles'"));
}

TEST_F(LaunchTest, StartWithFirstProfileAutoselected) {
  mock.MockReadFile(paths.kTasksConfig, tasks_config_with_profiles_data.dump());
  EXPECT_TRUE(InitTestCdt());
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(
      "Using profile \033[32mprofile 1\033[0m"));
  EXPECT_THAT(actual, testing::AllOf(
      testing::HasSubstr("build for macos with profile profile 1"),
      testing::HasSubstr("run on macos")
  )) << "task names formated according to profile";
}

TEST_F(LaunchTest, StartWithSpecifiedProfileSelected) {
  mock.MockReadFile(paths.kTasksConfig, tasks_config_with_profiles_data.dump());
  EXPECT_TRUE(InitTestCdt(profile2));
  std::string actual = out.str();
  EXPECT_THAT(actual, testing::HasSubstr(
      "Using profile \033[32mprofile 2\033[0m"));
  EXPECT_THAT(actual, testing::AllOf(
      testing::HasSubstr("build for windows with profile profile 2"),
      testing::HasSubstr("run on windows")
  )) << "task names formated according to profile";
}
