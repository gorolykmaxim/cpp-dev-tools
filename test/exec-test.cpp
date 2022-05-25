#include "test-lib.h"

class ExecTest: public CdtTest {};

TEST_F(ExecTest, StartAndListExecutions) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("exec");
}

TEST_F(ExecTest, StartExecuteTwoTasksAndListExecutions) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("t1");
  EXPECT_CMD("exec");
  EXPECT_CMD("exec0");
  EXPECT_CMD("exec99");
}

TEST_F(ExecTest, StartExecuteTwoTasksSelectFirstExecutionAndOpenFileLinksFromIt) {
  std::deque<ProcessExec>& proc_execs = mock.cmd_to_process_execs[execs.kHelloWorld];
  proc_execs.push_back(proc_execs.front());
  proc_execs.front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("t1");
  EXPECT_CMD("exec2");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteTwoTasksSelectFirstExecutionAndSearchItsOutput) {
  std::deque<ProcessExec>& proc_execs = mock.cmd_to_process_execs[execs.kHelloWorld];
  proc_execs.push_back(proc_execs.front());
  proc_execs.front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("t1");
  EXPECT_CMD("exec2");
  EXPECT_CMD("s\n(some|data)");
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndViewGtestOutput) {
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("t8");
  EXPECT_CMD("exec2");
  EXPECT_CMD("g0");
  EXPECT_CMD("g99");
  EXPECT_CMD("g");
  EXPECT_CMD("g1");
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndSearchGtestOutput) {
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("t8");
  EXPECT_CMD("exec2");
  EXPECT_CMD("gs");
  EXPECT_CMD("gs0");
  EXPECT_CMD("gs99");
  EXPECT_CMD("gs1\n(some|data)");
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtest) {
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("t8");
  EXPECT_CMD("exec2");
  EXPECT_CMD("gt");
  EXPECT_CMD("gt0");
  EXPECT_CMD("gt99");
  EXPECT_CMD("gt1");
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtestUntilItFails) {
  std::string e = execs.kTests + " --gtest_filter='failed_test_suit_1.test1'";
  mock.cmd_to_process_execs[e].push_back(successful_rerun_gtest_exec);
  mock.cmd_to_process_execs[e].push_back(successful_rerun_gtest_exec);
  mock.cmd_to_process_execs[e].push_back(failed_rerun_gtest_exec);
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("t8");
  EXPECT_CMD("exec2");
  EXPECT_CMD("gtr");
  EXPECT_CMD("gtr0");
  EXPECT_CMD("gtr99");
  EXPECT_CMD("gtr1");
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtestWithDebugger) {
  EXPECT_PROC(WITH_DEBUG("tests --gtest_filter='failed_test_suit_1.test1'"));
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("t8");
  EXPECT_CMD("exec2");
  EXPECT_CMD("gd");
  EXPECT_CMD("gd0");
  EXPECT_CMD("gd99");
  EXPECT_CMD("gd1");
}

TEST_F(ExecTest, StartExecuteTaskTwiceSelectFirstExecutionExecuteAnotherTaskSeeFirstTaskStillSelectedSelectLastExecutionExecuteAnotherTaskAndSeeItBeingSelectedAutomatically) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("t1");
  EXPECT_CMD("exec2");
  EXPECT_CMD("exec");
  EXPECT_CMD("t1");
  EXPECT_CMD("exec");
  EXPECT_CMD("exec1");
  EXPECT_CMD("t1");
  EXPECT_CMD("exec");
}

TEST_F(ExecTest, StartExecuteGtestTaskTwiceExecuteNormalTask110TimesWhileSelectingFirstNormalExecutionAndViewHistoryOf100ExecutionsThatIncludesOnlyOneLastGtestTaskAndSelectedFirstNormalExecution) {
  EXPECT_CDT_STARTED();
  for (int i = 0; i < 2; i++) {
      EXPECT_CMD("t8");
  }
  for (int i = 0; i < 100; i++) {
      EXPECT_CMD("t1");
      if (i == 1) {
          EXPECT_CMD("exec2");
      }
  }
  EXPECT_CMD("exec");
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskAndSearchItsOutput) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t3");
  EXPECT_CMD("exec3");
  EXPECT_CMD("s\npre pre task");
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskAndOpenLinkFromItsOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t3");
  EXPECT_CMD("exec3");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskThenResetSelectionAndOpenLinksFromTheNewestTaskOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre task 1"].front();
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t3");
  EXPECT_CMD("exec3");
  EXPECT_CMD("exec1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteGtestTaskWithPreTasksSelectOneOfPretasksWithLinksInOutputResetExecutionSelectionBackToTheLatestGtestTaskValidateThatThereAreNoLinksInItsOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("exec3");
  EXPECT_CMD("exec1");
  EXPECT_CMD("o1");
}

TEST_F(ExecTest, StartExecuteTaskOpenLinksFromOuputValidateExecutionHistoryHasOnlyOneExecution) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
  EXPECT_CMD("exec");
}
