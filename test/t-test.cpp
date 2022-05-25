#include "test-lib.h"

class TTest: public CdtTest {};

TEST_F(TTest, StartAndDisplayListOfTasksOnTaskCommand) {
  EXPECT_CDT_STARTED();
  // Display tasks list on no index
  EXPECT_CMD("t");
  // Display tasks list on non-existent task specified
  EXPECT_CMD("t0");
  EXPECT_CMD("t99");
}

TEST_F(TTest, StartAndExecuteSingleTask) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
}

TEST_F(TTest, StartAndExecuteTaskThatPrintsToStdoutAndStderr) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.output_lines = {"stdo", "stde", "ut\n", "rr\n"};
  exec.stderr_lines = {1, 3};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
}

TEST_F(TTest, StartAndExecuteTaskWithPreTasksWithPreTasks) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t2");
}

TEST_F(TTest, StartAndFailPrimaryTask) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.exit_code = 1;
  exec.output_lines = {"starting...\n", "error!!!\n"};
  exec.stderr_lines = {1};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
}

TEST_F(TTest, StartAndFailOneOfPreTasks) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 2"].front();
  exec.exit_code = 1;
  exec.output_lines = {"error!!!\n"};
  exec.stderr_lines = {0};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t2");
}
