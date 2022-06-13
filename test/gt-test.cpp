#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class GtTest: public CdtTest {};

TEST_F(GtTest, StartAttemptToRerunGtTestWhenNoTestsHaveBeenExecutedYet) {
  ASSERT_CDT_STARTED();
  CMD("gt");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksFailAttemptToRerunTestThatDoesNotExistAndViewListOfFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gt");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gt0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gt99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  // Check that we don't attempt to rerun any test
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     execs.kTests);
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksFailAndRerunFailedTest) {
  std::string pre_task_1 = "echo pre pre task 1";
  std::string pre_task_2 = "echo pre pre task 2";
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
  std::string first_test_rerun_cmd =
      execs.kTests + WITH_GT_FILTER("failed_test_suit_1.test1");
  std::string second_test_rerun_cmd =
      execs.kTests + WITH_GT_FILTER("failed_test_suit_2.test1");
  mock.cmd_to_process_execs[first_test_rerun_cmd].push_back(first_test_rerun);
  mock.cmd_to_process_execs[second_test_rerun_cmd].push_back(second_test_rerun);
  std::vector<std::string> first_test_rerun_expected_out = {
      OUT_LINKS_HIGHLIGHTED(), OUT_TEST_ERROR(),
      TASK_FAILED("failed_test_suit_1.test1", first_test_rerun.exit_code)};
  std::vector<std::string> second_test_rerun_expected_out = {
      OUT_TEST_ERROR(),
      TASK_FAILED("failed_test_suit_2.test1", second_test_rerun.exit_code)};
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gt1");
  EXPECT_OUT(HasSubstrsInOrder(first_test_rerun_expected_out));
  CMD("gt2");
  EXPECT_OUT(HasSubstrsInOrder(second_test_rerun_expected_out));
  EXPECT_PROCS_EXACT(pre_task_1, pre_task_2, execs.kTests,
                     pre_task_1, pre_task_2, first_test_rerun_cmd,
                     pre_task_1, pre_task_2, second_test_rerun_cmd);
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksSucceedAttemptToRerunTestThatDoesNotExistAndViewListOfAllTests) {
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gt");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gt0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gt99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  // Check that we don't attempt to rerun any test
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     execs.kTests);
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksSucceedAndRerunOneOfTest) {
  std::string pre_task_1 = "echo pre pre task 1";
  std::string pre_task_2 = "echo pre pre task 2";
  std::string first_test_rerun_cmd =
      execs.kTests + WITH_GT_FILTER("test_suit_1.test1");
  std::string second_test_rerun_cmd =
      execs.kTests + WITH_GT_FILTER("test_suit_1.test2");
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
  mock.cmd_to_process_execs[second_test_rerun_cmd].push_back(second_test_rerun);
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gt1");
  EXPECT_OUT(HasSubstr(OUT_LINKS_HIGHLIGHTED()));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("test_suit_1.test1")));
  CMD("gt2");
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("test_suit_1.test2")));
  EXPECT_PROCS_EXACT(pre_task_1, pre_task_2, execs.kTests,
                     pre_task_1, pre_task_2, first_test_rerun_cmd,
                     pre_task_1, pre_task_2, second_test_rerun_cmd);
}
