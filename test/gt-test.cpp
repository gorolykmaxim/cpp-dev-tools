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
    "Running main() from /lib/gtest_main.cc\n",
    "Note: Google Test filter = failed_test_suit_2.test1\n",
    "[==========] Running 1 test from 1 test suite.\n",
    "[----------] Global test environment set-up.\n",
    "[----------] 1 test from failed_test_suit_2\n",
    "[ RUN      ] failed_test_suit_2.test1\n",
    out_test_error,
    "[  FAILED  ] failed_test_suit_2.test1 (0 ms)\n",
    "[----------] 1 test from failed_test_suit_2 (0 ms total)\n\n",
    "[----------] Global test environment tear-down\n",
    "[==========] 1 test from 1 test suite ran. (0 ms total)\n",
    "[  PASSED  ] 0 tests.\n",
    "[  FAILED  ] 1 test, listed below:\n",
    "[  FAILED  ] failed_test_suit_2.test1\n\n",
    " 1 FAILED TEST\n",
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
    "Running main() from /lib/gtest_main.cc\n",
    "Note: Google Test filter = test_suit_1.test2\n",
    "[==========] Running 1 test from 1 test suite.\n",
    "[----------] Global test environment set-up.\n",
    "[----------] 1 test from test_suit_1\n",
    "[ RUN      ] test_suit_1.test2\n",
    "[       OK ] test_suit_1.test2 (0 ms)\n",
    "[----------] 1 test from test_suit_1 (0 ms total)\n\n",
    "[----------] Global test environment tear-down\n",
    "[==========] 1 test from 1 test suite ran. (0 ms total)\n",
    "[  PASSED  ] 1 test.\n",
  };
  std::string exec = execs.kTests + WITH_GT_FILTER("test_suit_1.test2");
  mock.cmd_to_process_execs[exec].push_back(second_test_rerun);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gt1");
  EXPECT_CMD("gt2");
}
