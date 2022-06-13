#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class ExecTest: public CdtTest {};

TEST_F(ExecTest, StartAndListExecutions) {
  ASSERT_CDT_STARTED();
  CMD("exec");
  EXPECT_OUT(HasSubstr("No task has been executed yet"));
}

TEST_F(ExecTest, StartExecuteTwoTasksAndListExecutions) {
  std::vector<std::string> expected_tasks = {"pre pre task 1", "hello world!"};
  std::string hello_world_selected_regex = "-> 1 ..:00:02 \"hello world!\"";
  ASSERT_CDT_STARTED();
  CMD("t5");
  CMD("t1");
  CMD("exec");
  EXPECT_OUT(HasSubstrsInOrder(expected_tasks));
  EXPECT_OUT(ContainsRegex(hello_world_selected_regex));
  CMD("exec0");
  EXPECT_OUT(HasSubstrsInOrder(expected_tasks));
  EXPECT_OUT(ContainsRegex(hello_world_selected_regex));
  CMD("exec99");
  EXPECT_OUT(HasSubstrsInOrder(expected_tasks));
  EXPECT_OUT(ContainsRegex(hello_world_selected_regex));
}

TEST_F(ExecTest, StartExecuteTwoTasksSelectFirstExecutionAndOpenFileLinksFromIt) {
  std::deque<ProcessExec>& procs = mock.cmd_to_process_execs[execs.kHelloWorld];
  procs.push_back(procs.front());
  procs.front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t1");
  CMD("t1");
  CMD("exec2");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteTwoTasksSelectFirstExecutionAndSearchItsOutput) {
  std::deque<ProcessExec>& procs = mock.cmd_to_process_execs[execs.kHelloWorld];
  procs.push_back(procs.front());
  procs.front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t1");
  CMD("t1");
  CMD("exec2");
  CMD("s\n(some|data)");
  EXPECT_OUT(HasSubstr(
      "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
      "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndViewGtestOutput) {
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("t8");
  CMD("exec2");
  CMD("g0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("g99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("g");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("g1");
  EXPECT_OUT(HasSubstr(OUT_LINKS_HIGHLIGHTED()));
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndSearchGtestOutput) {
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("t8");
  CMD("exec2");
  CMD("gs");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gs0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gs99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gs1\n(some|data)");
  EXPECT_OUT(HasSubstr(
      "\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
      "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtest) {
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("t8");
  CMD("exec2");
  CMD("gt");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gt0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gt99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gt1");
  EXPECT_PROCS_EXACT(execs.kTests, execs.kTests,
                     execs.kTests + WITH_GT_FILTER("test_suit_1.test1"));
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtestUntilItFails) {
  std::string rerun = execs.kTests + WITH_GT_FILTER("failed_test_suit_1.test1");
  mock.cmd_to_process_execs[rerun].push_back(successful_rerun_gtest_exec);
  mock.cmd_to_process_execs[rerun].push_back(successful_rerun_gtest_exec);
  mock.cmd_to_process_execs[rerun].push_back(failed_rerun_gtest_exec);
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("t8");
  CMD("exec2");
  CMD("gtr");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gtr0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gtr99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gtr1");
  EXPECT_PROCS_EXACT(execs.kTests, execs.kTests, rerun, rerun, rerun);
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtestWithDebugger) {
  std::string exec =
      WITH_DEBUG("tests" WITH_GT_FILTER("failed_test_suit_1.test1"));
  mock.cmd_to_process_execs[exec].push_back(ProcessExec{});
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("t8");
  CMD("exec2");
  CMD("gd");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gd0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gd99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gd1");
  EXPECT_PROCS_EXACT(execs.kTests, execs.kTests, exec);
}

TEST_F(ExecTest, StartExecuteTaskTwiceSelectFirstExecutionExecuteAnotherTaskSeeFirstTaskStillSelectedSelectLastExecutionExecuteAnotherTaskAndSeeItBeingSelectedAutomatically) {
  ASSERT_CDT_STARTED();
  CMD("t1");
  CMD("t1");
  CMD("exec2");
  CMD("exec");
  EXPECT_OUT(ContainsRegex("-> 2 ..:00:01 \"hello world!\""));
  EXPECT_OUT(HasSubstrNTimes("->", 1));
  CMD("t1");
  CMD("exec");
  EXPECT_OUT(ContainsRegex("-> 3 ..:00:01 \"hello world!\""));
  EXPECT_OUT(HasSubstrNTimes("->", 1));
  CMD("exec1");
  CMD("t1");
  CMD("exec");
  EXPECT_OUT(ContainsRegex("-> 1 ..:00:04 \"hello world!\""));
  EXPECT_OUT(HasSubstrNTimes("->", 1));
}

TEST_F(ExecTest, StartExecuteGtestTaskTwiceExecuteNormalTask110TimesWhileSelectingFirstNormalExecutionAndViewHistoryOf100ExecutionsThatIncludesOnlyOneLastGtestTaskAndSelectedFirstNormalExecution) {
  ASSERT_CDT_STARTED();
  for (int i = 0; i < 2; i++) {
      CMD("t8");
  }
  for (int i = 0; i < 110; i++) {
      CMD("t1");
      if (i == 1) {
          CMD("exec2");
      }
  }
  CMD("exec");
  // Last __gtest task execution does not get deleted
  EXPECT_OUT(ContainsRegex("100 ..:..:02 \"run tests\""));
  // Selected execution does not get deleted
  EXPECT_OUT(ContainsRegex("-> 99 ..:..:03 \"hello world!\""));
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskAndSearchItsOutput) {
  ASSERT_CDT_STARTED();
  CMD("t3");
  CMD("exec3");
  CMD("s\npre pre task");
  EXPECT_OUT(HasSubstr("\x1B[35m1:\x1B[0m\x1B[32mpre pre task\x1B[0m 1"));
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskAndOpenLinkFromItsOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t3");
  CMD("exec3");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskThenResetSelectionAndOpenLinksFromTheNewestTaskOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre task 1"].front();
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t3");
  CMD("exec3");
  CMD("exec1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteGtestTaskWithPreTasksSelectOneOfPretasksWithLinksInOutputResetExecutionSelectionBackToTheLatestGtestTaskValidateThatThereAreNoLinksInItsOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("exec3");
  CMD("exec1");
  CMD("o1");
  EXPECT_NOT_PROCS_LIKE(execs.kEditor);
}

TEST_F(ExecTest, StartExecuteTaskOpenLinksFromOuputValidateExecutionHistoryHasOnlyOneExecution) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
  CMD("exec");
  // Execution history has only one line and its the first execution
  EXPECT_OUT(ContainsRegex("\n-> 1 ..:00:01 \"hello world!\"\n"));
  EXPECT_OUT(HasSubstrNTimes("\n", 2));
}
