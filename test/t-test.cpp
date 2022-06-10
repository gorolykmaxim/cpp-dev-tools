#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace testing;

class TTest: public CdtTest {
public:
  Task buildOnMacos, buildOnWindows, runVariableBinary;

  void AppendTaskWithVariablePreTaskName() {
    nlohmann::json& tasks_list = tasks_config_with_profiles_data["cdt_tasks"];
    buildOnMacos = CreateTaskAndProcess(tasks_list, "build on macos");
    buildOnWindows = CreateTaskAndProcess(tasks_list, "build on windows");
    runVariableBinary = CreateTaskAndProcess(tasks_list, "run variable binary",
                                             {"build on {platform}"});
  }
};

TEST_F(TTest, StartAndDisplayListOfTasksOnTaskCommand) {
  ASSERT_CDT_STARTED();
  // Display tasks list on no index
  EXPECT_CMDOUT("t", HasSubstrsInOrder(list_of_tasks_in_ui));
  // Display tasks list on non-existent task specified
  EXPECT_CMDOUT("t0", HasSubstrsInOrder(list_of_tasks_in_ui));
  EXPECT_CMDOUT("t99", HasSubstrsInOrder(list_of_tasks_in_ui));
}

TEST_F(TTest, StartAndExecuteSingleTask) {
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t1", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_TASK(tasks.helloWorld), tasks.helloWorld.name,
      TASK_COMPLETE(tasks.helloWorld)}));
  EXPECT_PROCS_EXACT(execs.kHelloWorld);
}

TEST_F(TTest, StartAndExecuteTaskThatPrintsToStdoutAndStderr) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.append_eol = false;
  exec.output_lines = {"stdo", "stde", "ut" + kEol, "rr" + kEol};
  exec.stderr_lines = {1, 3};
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t1", HasSubstr("stdout\nstderr\n"));
}

TEST_F(TTest, StartAndExecuteTaskWithPreTasksWithPreTasks) {
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t2", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK(tasks.prePreTask1), RUNNING_PRE_TASK(tasks.prePreTask2),
      RUNNING_PRE_TASK(tasks.preTask1), RUNNING_PRE_TASK(tasks.preTask2),
      RUNNING_TASK(tasks.primaryTask), tasks.primaryTask.name,
      TASK_COMPLETE(tasks.primaryTask)}));
  EXPECT_PROCS_EXACT(tasks.prePreTask1.command, tasks.prePreTask2.command,
                     tasks.preTask1.command, tasks.preTask2.command,
                     tasks.primaryTask.command);
}

TEST_F(TTest, StartAndFailPrimaryTask) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.exit_code = 1;
  exec.output_lines = {"starting...", "error!!!"};
  exec.stderr_lines = {1};
  std::vector<std::string> expected_msgs;
  expected_msgs.push_back(RUNNING_TASK(tasks.helloWorld));
  expected_msgs.insert(expected_msgs.end(), exec.output_lines.begin(),
                       exec.output_lines.end());
  expected_msgs.push_back(TASK_FAILED(tasks.helloWorld, exec.exit_code));
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t1", HasSubstrsInOrder(expected_msgs));
}

TEST_F(TTest, StartAndFailOneOfPreTasks) {
  ProcessExec& exec = mock.cmd_to_process_execs[tasks.prePreTask2.command]
      .front();
  exec.exit_code = 1;
  exec.output_lines = {"error!!!"};
  exec.stderr_lines = {0};
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t2", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK(tasks.prePreTask1), RUNNING_PRE_TASK(tasks.prePreTask2),
      exec.output_lines.front(),
      TASK_FAILED(tasks.prePreTask2, exec.exit_code)}));
  EXPECT_PROCS_EXACT(tasks.prePreTask1.command, tasks.prePreTask2.command);
}

TEST_F(TTest, StartAndExecuteTaskWithProfile1) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  EXPECT_CMDOUT("t11",
                HasSubstr(tasks.buildForPlatformWithProfile[profile1].name));
  EXPECT_PROCS_EXACT(tasks.buildForPlatformWithProfile[profile1].command);
}

TEST_F(TTest, StartAndExecuteTaskWithProfile2) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  EXPECT_CMDOUT("t11",
                HasSubstr(tasks.buildForPlatformWithProfile[profile2].name));
  EXPECT_PROCS_EXACT(tasks.buildForPlatformWithProfile[profile2].command);
}

TEST_F(TTest, StartAndExecuteTaskWithProfile1PreTask) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  EXPECT_CMDOUT("t12", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK(tasks.buildForPlatformWithProfile[profile1]),
      RUNNING_TASK(tasks.runOnPlatform[profile1]),
      tasks.runOnPlatform[profile1].name,
      TASK_COMPLETE(tasks.runOnPlatform[profile1])}));
  EXPECT_PROCS_EXACT(tasks.buildForPlatformWithProfile[profile1].command,
                     tasks.runOnPlatform[profile1].command);
}

TEST_F(TTest, StartAndExecuteTaskWithProfile2PreTask) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  EXPECT_CMDOUT("t12", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK(tasks.buildForPlatformWithProfile[profile2]),
      RUNNING_TASK(tasks.runOnPlatform[profile2]),
      tasks.runOnPlatform[profile2].name,
      TASK_COMPLETE(tasks.runOnPlatform[profile2])}));
  EXPECT_PROCS_EXACT(tasks.buildForPlatformWithProfile[profile2].command,
                     tasks.runOnPlatform[profile2].command);
}

TEST_F(TTest, StartAndExecuteTaskWithPreTaskNameOfWhichIsDefinedInProfile1) {
  AppendTaskWithVariablePreTaskName();
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  EXPECT_CMDOUT("t15", HasSubstr(RUNNING_PRE_TASK(buildOnMacos)));
  EXPECT_PROCS_EXACT(buildOnMacos.command, runVariableBinary.command);
}

TEST_F(TTest, StartAndExecuteTaskWithPreTaskNameOfWhichIsDefinedInProfile2) {
  AppendTaskWithVariablePreTaskName();
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  EXPECT_CMDOUT("t15", HasSubstr(RUNNING_PRE_TASK(buildOnWindows)));
  EXPECT_PROCS_EXACT(buildOnWindows.command, runVariableBinary.command);
}

TEST_F(TTest, StartAndFailToExecuteTaskDueToFailureToLaunchProcess) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().fail_to_exec = true;
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t1", HasSubstr("Failed to exec: " + execs.kHelloWorld));
  EXPECT_PROCS_EXACT(execs.kHelloWorld);
}
