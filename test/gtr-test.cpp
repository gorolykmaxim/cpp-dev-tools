#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class GtrTest: public CdtTest {};

TEST_F(GtrTest, StartRepeatedlyExecuteGtestTaskWithPreTasksUntilItFailsAndRepeatedlyRerunFailedTestUntilItFails) {
  std::string pre_task1 = "echo pre pre task 1";
  std::string pre_task2 = "echo pre pre task 2";
  std::string out_links = OUT_LINKS_HIGHLIGHTED();
  std::string task_complete = TASK_COMPLETE("failed_test_suit_1.test1");
  std::vector<std::string> expected_out = {
      out_links, task_complete, out_links, task_complete, out_links,
      OUT_TEST_ERROR(), TASK_FAILED("failed_test_suit_1.test1",
                                    failed_rerun_gtest_exec.exit_code)};
  std::string exec = execs.kTests + WITH_GT_FILTER("failed_test_suit_1.test1");
  mock.cmd_to_process_execs[exec].push_back(successful_rerun_gtest_exec);
  mock.cmd_to_process_execs[exec].push_back(successful_rerun_gtest_exec);
  mock.cmd_to_process_execs[exec].push_back(failed_rerun_gtest_exec);
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  ASSERT_CDT_STARTED();
  CMD("tr10");
  CMD("gtr1");
  EXPECT_OUT(HasSubstrsInOrder(expected_out));
  EXPECT_PROCS_EXACT(pre_task1, pre_task2, execs.kTests, execs.kTests,
                     execs.kTests, pre_task1, pre_task2, exec, exec, exec);
}

TEST_F(GtrTest, StartExecuteGtestTaskWithPreTasksSucceedRepeatedlyRerunOneOfTheTestsAndFailDueToFailedPreTask) {
  std::string pre_task1 = "echo pre pre task 1";
  std::string pre_task2 = "echo pre pre task 2";
  ProcessExec failed_pre_task = mock.cmd_to_process_execs["echo pre pre task 1"]
      .front();
  failed_pre_task.exit_code = 1;
  mock.cmd_to_process_execs["echo pre pre task 1"].push_back(failed_pre_task);
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gtr1");
  EXPECT_OUT(HasSubstr(TASK_FAILED("pre pre task 1",
                                   failed_pre_task.exit_code)));
  EXPECT_PROCS_EXACT(pre_task1, pre_task2, execs.kTests, pre_task1);
}

TEST_F(GtrTest, StartExecuteLongGtestTaskWithPreTasksAbortItRepeatedlyRerunFailedTestAndAbortItAgain) {
  std::string pre_task1 = "echo pre pre task 1";
  std::string pre_task2 = "echo pre pre task 2";
  ProcessExec rerun;
  rerun.exit_code = 1;
  rerun.is_long = true;
  rerun.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "Note: Google Test filter = exit_tests.exit_in_the_middle",
    "[==========] Running 1 test from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from exit_tests",
    "[ RUN      ] exit_tests.exit_in_the_middle",
  };
  aborted_gtest_exec.is_long = true;
  mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
  std::string exec =
      execs.kTests + WITH_GT_FILTER("exit_tests.exit_in_the_middle");
  mock.cmd_to_process_execs[exec].push_back(rerun);
  ASSERT_CDT_STARTED();
  INTERRUPT_CMD("t10");
  INTERRUPT_CMD("gtr1");
  EXPECT_OUT(HasSubstr(TASK_FAILED("exit_tests.exit_in_the_middle", -1)));
  EXPECT_PROCS_EXACT(pre_task1, pre_task2, execs.kTests, pre_task1, pre_task2,
                     exec);
}

TEST_F(GtrTest, StartAttemptToRepeatedlyRerunGtestWhenNoTestsHaveBeenExecutedYet) {
  ASSERT_CDT_STARTED();
  CMD("gtr");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(GtrTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRepeatedlyRerunTestThatDoesNotExistAndViewListOfAllTests) {
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gtr");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gtr0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gtr99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  // Check that we don't attempt to rerun any test
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     execs.kTests);
}

TEST_F(GtrTest, StartExecuteGtestTaskRerunOneOfItsTestsAndDisplayListOfOriginallyExecutedTests) {
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("gt1");
  CMD("g");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
}
