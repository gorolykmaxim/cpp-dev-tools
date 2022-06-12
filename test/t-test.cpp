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
  CMD("t");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks_in_ui));
  // Display tasks list on non-existent task specified
  CMD("t0");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks_in_ui));
  CMD("t99");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks_in_ui));
}

TEST_F(TTest, StartAndExecuteSingleTask) {
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_OUT(HasSubstr("hello world!"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("hello world!")));
  EXPECT_PROCS_EXACT(execs.kHelloWorld);
}

TEST_F(TTest, StartAndExecuteTaskThatPrintsToStdoutAndStderr) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.append_eol = false;
  exec.output_lines = {"stdo", "stde", "ut" + kEol, "rr" + kEol};
  exec.stderr_lines = {1, 3};
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_OUT(HasSubstr("stdout\nstderr\n"));
}

TEST_F(TTest, StartAndExecuteTaskWithPreTasksWithPreTasks) {
  ASSERT_CDT_STARTED();
  CMD("t2");
  EXPECT_OUT(HasSubstr("primary task"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("primary task")));
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     "echo pre task 1", "echo pre task 2", "echo primary task");
}

TEST_F(TTest, StartAndFailPrimaryTask) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.exit_code = 1;
  exec.output_lines = {"starting...", "error!!!"};
  exec.stderr_lines = {1};
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_OUT(HasSubstrsInOrder(exec.output_lines));
  EXPECT_OUT(HasSubstr(TASK_FAILED("hello world!", exec.exit_code)));
}

TEST_F(TTest, StartAndFailOneOfPreTasks) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 2"].front();
  exec.exit_code = 1;
  exec.output_lines = {"error!!!"};
  exec.stderr_lines = {0};
  ASSERT_CDT_STARTED();
  CMD("t2");
  EXPECT_OUT(HasSubstr(exec.output_lines.front()));
  EXPECT_OUT(HasSubstr(TASK_FAILED("pre pre task 2", exec.exit_code)));
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2");
}

TEST_F(TTest, StartAndExecuteTaskWithProfile1) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  CMD("t11");
  EXPECT_OUT(HasSubstr("build for macos with profile profile 1"));
  EXPECT_PROCS_EXACT("echo build for macos with profile profile 1");
}

TEST_F(TTest, StartAndExecuteTaskWithProfile2) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  CMD("t11");
  EXPECT_OUT(HasSubstr("build for windows with profile profile 2"));
  EXPECT_PROCS_EXACT("echo build for windows with profile profile 2");
}

TEST_F(TTest, StartAndExecuteTaskWithProfile1PreTask) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  CMD("t12");
  EXPECT_OUT(HasSubstr("run on macos"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run on macos")));
  EXPECT_PROCS_EXACT("echo build for macos with profile profile 1",
                     "echo run on macos");
}

TEST_F(TTest, StartAndExecuteTaskWithProfile2PreTask) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  CMD("t12");
  EXPECT_OUT(HasSubstr("run on windows"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run on windows")));
  EXPECT_PROCS_EXACT("echo build for windows with profile profile 2",
                     "echo run on windows");
}

TEST_F(TTest, StartAndExecuteTaskWithPreTaskNameOfWhichIsDefinedInProfile1) {
  AppendTaskWithVariablePreTaskName();
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  CMD("t15");
  EXPECT_OUT(HasSubstr("build on macos"));
  EXPECT_PROCS_EXACT("echo build on macos", "echo run variable binary");
}

TEST_F(TTest, StartAndExecuteTaskWithPreTaskNameOfWhichIsDefinedInProfile2) {
  AppendTaskWithVariablePreTaskName();
  ASSERT_CDT_STARTED_WITH_PROFILE(profile2);
  CMD("t15");
  EXPECT_OUT(HasSubstr("build on windows"));
  EXPECT_PROCS_EXACT("echo build on windows", "echo run variable binary");
}

TEST_F(TTest, StartAndFailToExecuteTaskDueToFailureToLaunchProcess) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().fail_to_exec = true;
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_OUT(HasSubstr("Failed to exec: " + execs.kHelloWorld));
  EXPECT_PROCS_EXACT(execs.kHelloWorld);
}
