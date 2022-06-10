#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace testing;

class TTest: public CdtTest {
public:
  void AppendTaskWithVariablePreTaskName() {
    nlohmann::json& tasks = tasks_config_with_profiles_data["cdt_tasks"];
    tasks.push_back(CreateTaskAndProcess("build on macos"));
    tasks.push_back(CreateTaskAndProcess("build on windows"));
    tasks.push_back(CreateTaskAndProcess("run variable binary",
                                         {"build on {platform}"}));
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
      RUNNING_TASK("hello world!"), "hello world!",
      TASK_COMPLETE("hello world!")}));
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
      RUNNING_PRE_TASK("pre pre task 1"), RUNNING_PRE_TASK("pre pre task 2"),
      RUNNING_PRE_TASK("pre task 1"), RUNNING_PRE_TASK("pre task 2"),
      RUNNING_TASK("primary task"), "primary task",
      TASK_COMPLETE("primary task")}));
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     "echo pre task 1", "echo pre task 2", "echo primary task");
}

TEST_F(TTest, StartAndFailPrimaryTask) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.exit_code = 1;
  exec.output_lines = {"starting...", "error!!!"};
  exec.stderr_lines = {1};
  std::vector<std::string> expected_msgs;
  expected_msgs.push_back(RUNNING_TASK("hello world!"));
  expected_msgs.insert(expected_msgs.end(), exec.output_lines.begin(),
                       exec.output_lines.end());
  expected_msgs.push_back(TASK_FAILED("hello world!", exec.exit_code));
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t1", HasSubstrsInOrder(expected_msgs));
}

TEST_F(TTest, StartAndFailOneOfPreTasks) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 2"].front();
  exec.exit_code = 1;
  exec.output_lines = {"error!!!"};
  exec.stderr_lines = {0};
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t2", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK("pre pre task 1"), RUNNING_PRE_TASK("pre pre task 2"),
      "error!!!", TASK_FAILED("pre pre task 2", exec.exit_code)}));
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2");
}

TEST_F(TTest, StartAndExecuteTaskWithProfile1) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  EXPECT_CMDOUT("t11", HasSubstr("build for macos with profile profile 1"));
  EXPECT_PROCS_EXACT("echo build for macos with profile profile 1");
}

TEST_F(TTest, StartAndExecuteTaskWithProfile2) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  EXPECT_CMDOUT("t11", HasSubstr("build for windows with profile profile 2"));
  EXPECT_PROCS_EXACT("echo build for windows with profile profile 2");
}

TEST_F(TTest, StartAndExecuteTaskWithProfile1PreTask) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  EXPECT_CMDOUT("t12", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK("build for macos with profile profile 1"),
      RUNNING_TASK("run on macos"), "run on macos",
      TASK_COMPLETE("run on macos")}));
  EXPECT_PROCS_EXACT("echo build for macos with profile profile 1",
                     "echo run on macos");
}

TEST_F(TTest, StartAndExecuteTaskWithProfile2PreTask) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  EXPECT_CMDOUT("t12", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK("build for windows with profile profile 2"),
      RUNNING_TASK("run on windows"), "run on windows",
      TASK_COMPLETE("run on windows")}));
  EXPECT_PROCS_EXACT("echo build for windows with profile profile 2",
                     "echo run on windows");
}

TEST_F(TTest, StartAndExecuteTaskWithPreTaskNameOfWhichIsDefinedInProfile1) {
  AppendTaskWithVariablePreTaskName();
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  EXPECT_CMDOUT("t15", HasSubstr(RUNNING_PRE_TASK("build on macos")));
  EXPECT_PROCS_EXACT("echo build on macos", "echo run variable binary");
}

TEST_F(TTest, StartAndExecuteTaskWithPreTaskNameOfWhichIsDefinedInProfile2) {
  AppendTaskWithVariablePreTaskName();
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  EXPECT_CMDOUT("t15", HasSubstr(RUNNING_PRE_TASK("build on windows")));
  EXPECT_PROCS_EXACT("echo build on windows", "echo run variable binary");
}

TEST_F(TTest, StartAndFailToExecuteTaskDueToFailureToLaunchProcess) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().fail_to_exec = true;
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t1", HasSubstr("Failed to exec: " + execs.kHelloWorld));
  EXPECT_PROCS_EXACT(execs.kHelloWorld);
}
