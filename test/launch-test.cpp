#include "test-lib.h"

class LaunchTest : public CdtTest {};

TEST_F(LaunchTest, StartAndViewTasks) {
  EXPECT_CDT_STARTED();
}

TEST_F(LaunchTest, FailToStartDueToUserConfigNotBeingJson) {
  mock.MockReadFile(paths.kUserConfig, "not a json");
  EXPECT_CDT_ABORTED();
}

TEST_F(LaunchTest, FailToStartDueToUserConfigHavingPropertiesInIncorrectFormat) {
  nlohmann::json user_config_data;
  user_config_data["open_in_editor_command"] = "my-editor";
  user_config_data["debug_command"] = "my-debugger";
  mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
  EXPECT_CDT_ABORTED();
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotSpecified) {
  std::vector<const char*> argv = {execs.kCdt.c_str()};
  EXPECT_FALSE(InitCdt(argv.size(), argv.data(), cdt));
  EXPECT_EQ("\x1B[31musage: cpp-dev-tools tasks.json\n\x1B[0m", out.str());
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotExisting) {
  mock.MockReadFile(paths.kTasksConfig);
  EXPECT_CDT_ABORTED();
}

TEST_F(LaunchTest, FailToStartDueToTasksConfigNotBeingJson) {
  mock.MockReadFile(paths.kTasksConfig, "not a json");
  EXPECT_CDT_ABORTED();
}

TEST_F(LaunchTest, FailToStartDueToCdtTasksNotBeingSpecifiedInConfig) {
  mock.MockReadFile(paths.kTasksConfig, "{}");
  EXPECT_CDT_ABORTED();
}

TEST_F(LaunchTest, FailToStartDueToCdtTasksNotBeingArrayOfObjects) {
  nlohmann::json tasks_config_data;
  tasks_config_data["cdt_tasks"] = "string";
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_CDT_ABORTED();
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
  };
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  EXPECT_CDT_ABORTED();
}

TEST_F(LaunchTest, StartAndChangeCwdToTasksConfigsDirectory) {
  EXPECT_CALL(mock, SetCurrentPath(paths.kTasksConfig.parent_path()));
  EXPECT_CDT_STARTED();
}

TEST_F(LaunchTest, StartAndCreateExampleUserConfig) {
  std::string default_user_config_content =
      "{\n"
      "  // Open file links from the output in Sublime Text:\n"
      "  //\"open_in_editor_command\": \"subl {}\"\n"
      "  // Open file links from the output in VSCode:\n"
      "  //\"open_in_editor_command\": \"code {}\"\n"
      "  // Debug tasks on MacOS:\n"
      "  //\"debug_command\": \"osascript -e 'tell application \\\"Terminal\\\""
      " to do script \\\"cd {current_dir} && lldb -- {shell_cmd}\\\"'\"\n"
      "}\n";
  EXPECT_CALL(mock, FileExists(paths.kUserConfig))
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(mock, WriteFile(paths.kUserConfig, default_user_config_content));
  EXPECT_CDT_STARTED();
}

TEST_F(LaunchTest, StartAndNotOverrideExistingUserConfig) {
  EXPECT_CALL(mock, WriteFile(testing::Eq(paths.kUserConfig), testing::_))
      .Times(0);
  EXPECT_CDT_STARTED();
}
