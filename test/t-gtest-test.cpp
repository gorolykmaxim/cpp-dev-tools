#include "test-lib.h"

class TGtestTest: public CdtTest {};

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithNoTests) {
  mock.cmd_to_process_execs[execs.kTests].front().output_lines = {
    "Running main() from /lib/gtest_main.cc\n",
    "[==========] Running 0 tests from 0 test suites.\n",
    "[==========] 0 tests from 0 test suites ran. (0 ms total)\n",
    "[  PASSED  ] 0 tests.\n",
  };
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
}

TEST_F(TGtestTest, StartAttemptToExecuteGtestTaskWithNonExistentBinaryAndFail) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kTests].front();
  exec.exit_code = 127;
  exec.output_lines = {execs.kTests + " does not exist\n"};
  exec.stderr_lines = {0};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithNonGtestBinary) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  mock.cmd_to_process_execs[execs.kTests].front() = exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithNonGtestBinaryThatDoesNotFinishAndAbortItManually) {
  ProcessExec exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.is_long = true;
  mock.cmd_to_process_execs[execs.kTests].front() = exec;
  EXPECT_CDT_STARTED();
  EXPECT_INTERRUPTED_CMD("t8");
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskThatExitsWith0ExitCodeInTheMiddle) {
  aborted_gtest_exec.exit_code = 0;
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskThatExitsWith1ExitCodeInTheMiddle) {
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
}

TEST_F(TGtestTest, StartAttemptToExecuteTaskWithGtestPreTaskThatExitsWith0ExitCodeInTheMiddleAndFail) {
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t9");
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithMultipleSuites) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithMultipleSuitesEachHavingFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
}

TEST_F(TGtestTest, StartAndExecuteTaskWithGtestPreTaskWithMultipleSuites) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t9");
}

TEST_F(TGtestTest, StartAndExecuteTaskWithGtestPreTaskWithMultipleSuitesEachHavingFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t9");
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithLongTestAndAbortIt) {
  aborted_gtest_exec.is_long = true;
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_INTERRUPTED_CMD("t8");
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithFailedTestsAndLongTestAndAbortIt) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kTests].front();
  exec.exit_code = 1;
  exec.is_long = true;
  exec.output_lines = {
    "Running main() from /lib/gtest_main.cc\n",
    "[==========] Running 2 tests from 2 test suites.\n",
    "[----------] Global test environment set-up.\n",
    "[----------] 1 test from failed_test_suit_1\n",
    "[ RUN      ] failed_test_suit_1.test1\n",
    out_links,
    out_test_error,
    "[  FAILED  ] failed_test_suit_1.test1 (0 ms)\n",
    "[----------] 1 test from failed_test_suit_1 (0 ms total)\n\n",
    "[----------] 1 test from long_tests\n",
    "[ RUN      ] long_tests.test1\n"
  };
  EXPECT_CDT_STARTED();
  EXPECT_INTERRUPTED_CMD("t8");
}

TEST_F(TGtestTest, StartExecuteGtestTaskFailOneOfTheTestsAndViewAutomaticallyDisplayedTestOutput) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_single_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
}

TEST_F(TGtestTest, StartExecuteTaskWithGtestPreTaskFailOneOfTheTestsAndViewAutomaticallyDisplayedTestOutput) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_single_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t9");
}
