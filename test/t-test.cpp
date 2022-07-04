#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace testing;

class TTest: public CdtTest {
protected:
  std::string cmd_task1, cmd_task2;

  void SetUp() override {
    tasks = {
        CreateTask("task 1", "echo task 1"),
        CreateTask("task 2", "echo task 2", {"task 1"}),
        CreateTask("build for {platform} with profile {name}",
                   "echo build for {platform} with profile {name}"),
        CreateTask("run on {platform}", "echo run on {platform}",
                   {"build for {platform} with profile {name}"}),
        CreateTaskAndProcess("build on macos"),
        CreateTaskAndProcess("build on windows"),
        CreateTaskAndProcess("run variable binary", {"build on {platform}"})};
    profiles = {
        {{"name", "profile 1"}, {"platform", "macos"}},
        {{"name", "profile 2"}, {"platform", "windows"}}};
    cmd_task1 = tasks[0]["command"];
    cmd_task2 = tasks[1]["command"];
    CdtTest::SetUp();
  }

  std::string MockProc(const std::string& output) {
    std::string cmd = "echo " + output;
    ProcessExec exec;
    exec.output_lines = {output};
    mock.MockProc(cmd, exec);
    return cmd;
  }
};

TEST_F(TTest, StartAndDisplayListOfTasksOnTaskCommand) {
  tasks = {CreateTaskAndProcess("task 1"), CreateTaskAndProcess("task 2"),
           CreateTaskAndProcess("task 3")};
  std::vector<std::string> task_names = {"1 \"task 1\"", "2 \"task 2\"",
                                         "3 \"task 3\""};
  MockTasksConfig();
  ASSERT_INIT_CDT();
  // Display tasks list on no index
  RunCmd("t");
  EXPECT_OUT(HasSubstrsInOrder(task_names));
  // Display tasks list on non-existent task specified
  RunCmd("t0");
  EXPECT_OUT(HasSubstrsInOrder(task_names));
  RunCmd("t99");
  EXPECT_OUT(HasSubstrsInOrder(task_names));
}

TEST_F(TTest, StartAndExecuteSingleTask) {
  ProcessExec exec;
  exec.output_lines = {"task 1"};
  mock.MockProc(cmd_task1, exec);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstr("task 1"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("task 1")));
  EXPECT_PROCS_EXACT(cmd_task1);
}

TEST_F(TTest, StartAndExecuteTaskWithPreTasksWithPreTasks) {
  tasks = {
      CreateTaskAndProcess("pre pre task 1"),
      CreateTaskAndProcess("pre pre task 2"),
      CreateTaskAndProcess("pre task 1", {"pre pre task 1", "pre pre task 2"}),
      CreateTaskAndProcess("pre task 2"),
      CreateTaskAndProcess("primary task", {"pre task 1", "pre task 2"})};
  MockTasksConfig();
  ASSERT_INIT_CDT();
  RunCmd("t5");
  EXPECT_OUT(HasSubstr("primary task"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("primary task")));
  EXPECT_PROCS_EXACT(tasks[0]["command"], tasks[1]["command"],
                     tasks[2]["command"], tasks[3]["command"],
                     tasks[4]["command"]);
}

TEST_F(TTest, StartAndFailPrimaryTask) {
  ProcessExec exec;
  mock.MockProc(cmd_task1, exec);
  exec.exit_code = 1;
  exec.output_lines = {"starting...", "error!!!"};
  exec.stderr_lines = {1};
  mock.MockProc(cmd_task2, exec);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUT(HasSubstrsInOrder(exec.output_lines));
  EXPECT_OUT(HasSubstr(TASK_FAILED("task 2", exec.exit_code)));
  EXPECT_PROCS_EXACT(cmd_task1, cmd_task2);
}

TEST_F(TTest, StartAndFailOneOfPreTasks) {
  ProcessExec exec;
  mock.MockProc(cmd_task2, exec);
  exec.exit_code = 1;
  exec.output_lines = {"error!!!"};
  exec.stderr_lines = {0};
  mock.MockProc(cmd_task1, exec);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUT(HasSubstr(exec.output_lines.front()));
  EXPECT_OUT(HasSubstr(TASK_FAILED("task 1", exec.exit_code)));
  EXPECT_PROCS_EXACT(cmd_task1);
}

TEST_F(TTest, StartAndExecuteTaskWithProfile1) {
  std::string output = "build for macos with profile profile 1";
  std::string cmd = MockProc(output);
  args.push_back("profile 1");
  ASSERT_INIT_CDT();
  RunCmd("t3");
  EXPECT_OUT(HasSubstr(output));
  EXPECT_PROCS_EXACT(cmd);
}

TEST_F(TTest, StartAndExecuteTaskWithProfile2) {
  std::string output = "build for windows with profile profile 2";
  std::string cmd = MockProc(output);
  args.push_back("profile 2");
  ASSERT_INIT_CDT();
  RunCmd("t3");
  EXPECT_OUT(HasSubstr(output));
  EXPECT_PROCS_EXACT(cmd);
}

TEST_F(TTest, StartAndExecuteTaskWithProfile1PreTask) {
  std::string cmd_build = MockProc("build for macos with profile profile 1");
  std::string output = "run on macos";
  std::string cmd_run = MockProc(output);
  args.push_back("profile 1");
  ASSERT_INIT_CDT();
  RunCmd("t4");
  EXPECT_OUT(HasSubstr(output));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run on macos")));
  EXPECT_PROCS_EXACT(cmd_build, cmd_run);
}

TEST_F(TTest, StartAndExecuteTaskWithProfile2PreTask) {
  std::string cmd_build = MockProc("build for windows with profile profile 2");
  std::string output = "run on windows";
  std::string cmd_run = MockProc(output);
  args.push_back("profile 2");
  ASSERT_INIT_CDT();
  RunCmd("t4");
  EXPECT_OUT(HasSubstr(output));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run on windows")));
  EXPECT_PROCS_EXACT(cmd_build, cmd_run);
}

TEST_F(TTest, StartAndExecuteTaskWithPreTaskNameOfWhichIsDefinedInProfile1) {
  args.push_back("profile 1");
  ASSERT_INIT_CDT();
  RunCmd("t7");
  EXPECT_OUT(HasSubstr("build on macos"));
  EXPECT_PROCS_EXACT(tasks[4]["command"], tasks[6]["command"]);
}

TEST_F(TTest, StartAndExecuteTaskWithPreTaskNameOfWhichIsDefinedInProfile2) {
  args.push_back("profile 2");
  ASSERT_INIT_CDT();
  RunCmd("t7");
  EXPECT_OUT(HasSubstr("build on windows"));
  EXPECT_PROCS_EXACT(tasks[5]["command"], tasks[6]["command"]);
}

TEST_F(TTest, StartAndFailToExecuteTaskDueToFailureToLaunchProcess) {
  ProcessExec exec;
  exec.fail_to_exec_error = "No such file or directory";
  mock.MockProc(cmd_task1, exec);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstr("Failed to exec: " + cmd_task1));
  EXPECT_OUT(HasSubstr(exec.fail_to_exec_error));
  EXPECT_PROCS_EXACT(cmd_task1);
}
