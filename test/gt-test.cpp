#include "test-lib.h"
#include <string>

class GtTest: public CdtTest {};

TEST_F(GtTest, StartAttemptToRerunGtTestWhenNoTestsHaveBeenExecutedYet) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("gt");
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksFailAttemptToRerunTestThatDoesNotExistAndViewListOfFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gt");
  EXPECT_CMD("gt0");
  EXPECT_CMD("gt99");
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksFailAndRerunFailedTest) {
  ProcessExec first_test_rerun = failed_rerun_gtest_exec;
  ProcessExec second_test_rerun;
  second_test_rerun.exit_code = 1;
  second_test_rerun.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "Note: Google Test filter = failed_test_suit_2.test1",
    "[==========] Running 1 test from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from failed_test_suit_2",
    "[ RUN      ] failed_test_suit_2.test1",
    OUT_TEST_ERROR(),
    "[  FAILED  ] failed_test_suit_2.test1 (0 ms)",
    "[----------] 1 test from failed_test_suit_2 (0 ms total)",
    "",
    "[----------] Global test environment tear-down",
    "[==========] 1 test from 1 test suite ran. (0 ms total)",
    "[  PASSED  ] 0 tests.",
    "[  FAILED  ] 1 test, listed below:",
    "[  FAILED  ] failed_test_suit_2.test1",
    "",
    " 1 FAILED TEST",
  };
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  std::string one_test_exec;
  one_test_exec = execs.kTests + WITH_GT_FILTER("failed_test_suit_1.test1");
  mock.cmd_to_process_execs[one_test_exec].push_back(first_test_rerun);
  one_test_exec = execs.kTests + WITH_GT_FILTER("failed_test_suit_2.test1");
  mock.cmd_to_process_execs[one_test_exec].push_back(second_test_rerun);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gt1");
  EXPECT_CMD("gt2");
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksSucceedAttemptToRerunTestThatDoesNotExistAndViewListOfAllTests) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gt");
  EXPECT_CMD("gt0");
  EXPECT_CMD("gt99");
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksSucceedAndRerunOneOfTest) {
  ProcessExec second_test_rerun;
  second_test_rerun.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "Note: Google Test filter = test_suit_1.test2",
    "[==========] Running 1 test from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from test_suit_1",
    "[ RUN      ] test_suit_1.test2",
    "[       OK ] test_suit_1.test2 (0 ms)",
    "[----------] 1 test from test_suit_1 (0 ms total)",
    "",
    "[----------] Global test environment tear-down",
    "[==========] 1 test from 1 test suite ran. (0 ms total)",
    "[  PASSED  ] 1 test.",
  };
  std::string exec = execs.kTests + WITH_GT_FILTER("test_suit_1.test2");
  mock.cmd_to_process_execs[exec].push_back(second_test_rerun);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gt1");
  EXPECT_CMD("gt2");
}
