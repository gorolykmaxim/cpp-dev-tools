#include "test-lib.h"
#include <string>

class GtrTest: public CdtTest {};

TEST_F(GtrTest, StartRepeatedlyExecuteGtestTaskWithPreTasksUntilItFailsAndRepeatedlyRerunFailedTestUntilItFails) {
  std::string exec = execs.kTests + " --gtest_filter='failed_test_suit_1.test1'";
  mock.cmd_to_process_execs[exec].push_back(successful_rerun_gtest_exec);
  mock.cmd_to_process_execs[exec].push_back(successful_rerun_gtest_exec);
  mock.cmd_to_process_execs[exec].push_back(failed_rerun_gtest_exec);
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("tr10");
  EXPECT_CMD("gtr1");
}

TEST_F(GtrTest, StartExecuteGtestTaskWithPreTasksSucceedRepeatedlyRerunOneOfTheTestsAndFailDueToFailedPreTask) {
  ProcessExec failed_pre_task = mock.cmd_to_process_execs["echo pre pre task 1"]
      .front();
  failed_pre_task.exit_code = 1;
  mock.cmd_to_process_execs["echo pre pre task 1"].push_back(failed_pre_task);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gtr1");
}

TEST_F(GtrTest, StartExecuteLongGtestTaskWithPreTasksAbortItRepeatedlyRerunFailedTestAndAbortItAgain) {
  ProcessExec rerun;
  rerun.exit_code = 1;
  rerun.is_long = true;
  rerun.output_lines = {
    "Running main() from /lib/gtest_main.cc\n",
    "Note: Google Test filter = exit_tests.exit_in_the_middle\n",
    "[==========] Running 1 test from 1 test suite.\n",
    "[----------] Global test environment set-up.\n",
    "[----------] 1 test from exit_tests\n",
    "[ RUN      ] exit_tests.exit_in_the_middle\n",
  };
  aborted_gtest_exec.is_long = true;
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  std::string exec = execs.kTests + " --gtest_filter='exit_tests."
                     "exit_in_the_middle'";
  mock.cmd_to_process_execs[exec].push_back(rerun);
  EXPECT_CDT_STARTED();
  EXPECT_INTERRUPTED_CMD("t10");
  EXPECT_INTERRUPTED_CMD("gtr1");
}

TEST_F(GtrTest, StartAttemptToRepeatedlyRerunGtestWhenNoTestsHaveBeenExecutedYet) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("gtr");
}

TEST_F(GtrTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRepeatedlyRerunTestThatDoesNotExistAndViewListOfAllTests) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gtr");
  EXPECT_CMD("gtr0");
  EXPECT_CMD("gtr99");
}

TEST_F(GtrTest, StartExecuteGtestTaskRerunOneOfItsTestsAndDisplayListOfOriginallyExecutedTests) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("gt1");
  EXPECT_CMD("g");
}
