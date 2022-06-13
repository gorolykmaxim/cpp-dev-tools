#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class TGtestTest: public CdtTest {};

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithNoTests) {
  mock.cmd_to_process_execs[execs.kTests].front().output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "[==========] Running 0 tests from 0 test suites.",
    "[==========] 0 tests from 0 test suites ran. (0 ms total)",
    "[  PASSED  ] 0 tests.",
  };
  ASSERT_CDT_STARTED();
  CMD("t8");
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run tests")));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAttemptToExecuteGtestTaskWithNonExistentBinaryAndFail) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kTests].front();
  exec.exit_code = 127;
  exec.output_lines = {execs.kTests + " does not exist"};
  exec.stderr_lines = {0};
  ASSERT_CDT_STARTED();
  CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "'" + execs.kTests + "' is not a google test executable",
      TASK_FAILED("run tests", exec.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithNonGtestBinary) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  mock.cmd_to_process_execs[execs.kTests].front() = exec;
  ASSERT_CDT_STARTED();
  CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "'" + execs.kTests + "' is not a google test executable",
      TASK_FAILED("run tests", exec.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithNonGtestBinaryThatDoesNotFinishAndAbortItManually) {
  ProcessExec exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.is_long = true;
  mock.cmd_to_process_execs[execs.kTests].front() = exec;
  ASSERT_CDT_STARTED();
  INTERRUPT_CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "'" + execs.kTests + "' is not a google test executable",
      TASK_FAILED("run tests", -1)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskThatExitsWith0ExitCodeInTheMiddle) {
  aborted_gtest_exec.exit_code = 0;
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      TASK_FAILED("run tests", 0)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskThatExitsWith1ExitCodeInTheMiddle) {
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      TASK_FAILED("run tests", aborted_gtest_exec.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAttemptToExecuteTaskWithGtestPreTaskThatExitsWith0ExitCodeInTheMiddleAndFail) {
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t9");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      TASK_FAILED("run tests", aborted_gtest_exec.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithMultipleSuites) {
  ASSERT_CDT_STARTED();
  CMD("t8");
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run tests")));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithMultipleSuitesEachHavingFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  EXPECT_OUT(HasSubstr(TASK_FAILED("run tests", failed_gtest_exec.exit_code)));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteTaskWithGtestPreTaskWithMultipleSuites) {
  ASSERT_CDT_STARTED();
  CMD("t9");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "task with gtest pre task",
      TASK_COMPLETE("task with gtest pre task")}));
  EXPECT_PROCS_EXACT(execs.kTests, "echo task with gtest pre task");
}

TEST_F(TGtestTest, StartAndExecuteTaskWithGtestPreTaskWithMultipleSuitesEachHavingFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t9");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  EXPECT_OUT(HasSubstr(TASK_FAILED("run tests", failed_gtest_exec.exit_code)));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithLongTestAndAbortIt) {
  aborted_gtest_exec.is_long = true;
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  ASSERT_CDT_STARTED();
  INTERRUPT_CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      TASK_FAILED("run tests", -1)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithFailedTestsAndLongTestAndAbortIt) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kTests].front();
  exec.exit_code = 1;
  exec.is_long = true;
  exec.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "[==========] Running 2 tests from 2 test suites.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from failed_test_suit_1",
    "[ RUN      ] failed_test_suit_1.test1",
    OUT_LINKS_NOT_HIGHLIGHTED(),
    OUT_TEST_ERROR(),
    "[  FAILED  ] failed_test_suit_1.test1 (0 ms)",
    "[----------] 1 test from failed_test_suit_1 (0 ms total)",
    "",
    "[----------] 1 test from long_tests",
    "[ RUN      ] long_tests.test1"
  };
  ASSERT_CDT_STARTED();
  INTERRUPT_CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      "1 \"failed_test_suit_1.test1\"",
      "2 \"long_tests.test1\"",
      TASK_FAILED("run tests", -1)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteGtestTaskFailOneOfTheTestsAndViewAutomaticallyDisplayedTestOutput) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_single_gtest_exec;
  std::vector<std::string> expected_out = {
      OUT_LINKS_HIGHLIGHTED(), OUT_TEST_ERROR(),
      TASK_FAILED("run tests", failed_single_gtest_exec.exit_code)};
  ASSERT_CDT_STARTED();
  CMD("t8");
  EXPECT_OUT(HasSubstrsInOrder(expected_out));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteTaskWithGtestPreTaskFailOneOfTheTestsAndViewAutomaticallyDisplayedTestOutput) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_single_gtest_exec;
  std::vector<std::string> expected_out = {
      OUT_LINKS_HIGHLIGHTED(), OUT_TEST_ERROR(),
      TASK_FAILED("run tests", failed_single_gtest_exec.exit_code)};
  ASSERT_CDT_STARTED();
  CMD("t9");
  EXPECT_OUT(HasSubstrsInOrder(expected_out));
  EXPECT_PROCS_EXACT(execs.kTests);
}
