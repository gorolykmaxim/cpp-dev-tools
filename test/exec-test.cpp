#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class ExecTest: public CdtTest {
protected:
  std::vector<std::string> list_of_successful_tests, list_of_failed_tests;
  ProcessExec exec_tests_successful, exec_tests_failed;
  std::string cmd_test_rerun;
  std::vector<DummyTestSuite> suite_rerun;

  void SetUp() override {
    tasks = {
      CreateTask("task 1", "echo task 1"),
      CreateTaskAndProcess("task 2"),
      CreateTask("run tests", "__gtest " + execs.kTests),
      CreateTask("task 4", "echo task 4", {"task 1", "task 2"}),
      CreateTask("run tests after pre tasks", "__gtest " + execs.kTests,
                 {"task 1"}),
    };
    ProcessExec exec_task;
    exec_task.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    mock.MockProc("echo task 1", exec_task);
    mock.MockProc("echo task 4", exec_task);
    std::vector<DummyTestSuite> suites = {
        DummyTestSuite{"suite", {
            DummyTest{"test1", {OUT_LINKS_NOT_HIGHLIGHTED()}},
            DummyTest{"test2"}}}};
    list_of_successful_tests = {"1 \"suite.test1\"", "2 \"suite.test2\""};
    list_of_failed_tests = {"1 \"suite.test1\""};
    exec_tests_successful.output_lines = CreateTestOutput(suites);
    suites[0].tests[0].is_failed = true;
    exec_tests_failed.exit_code = 1;
    exec_tests_failed.output_lines = CreateTestOutput(suites);
    mock.MockProc(execs.kTests, exec_tests_successful);
    mock.MockProc(execs.kTests, exec_tests_failed);
    cmd_test_rerun = execs.kTests + WITH_GT_FILTER("suite.test1");
    suite_rerun = {DummyTestSuite{"suite", {DummyTest{"test1"}}}};
    Init();
  }
};

TEST_F(ExecTest, StartAndListExecutions) {
  ASSERT_INIT_CDT();
  RunCmd("exec");
  EXPECT_OUT(HasSubstr("No task has been executed yet"));
}

TEST_F(ExecTest, StartExecuteTwoTasksAndListExecutions) {
  std::vector<std::string> expected_tasks = {"task 2", "task 1"};
  std::string selected_regex = "-> 1 ..:00:02 \"task 1\"";
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("t1");
  RunCmd("exec");
  EXPECT_OUT(HasSubstrsInOrder(expected_tasks));
  EXPECT_OUT(ContainsRegex(selected_regex));
  RunCmd("exec0");
  EXPECT_OUT(HasSubstrsInOrder(expected_tasks));
  EXPECT_OUT(ContainsRegex(selected_regex));
  RunCmd("exec99");
  EXPECT_OUT(HasSubstrsInOrder(expected_tasks));
  EXPECT_OUT(ContainsRegex(selected_regex));
}

TEST_F(ExecTest, StartExecuteTwoTasksSelectFirstExecutionAndOpenFileLinksFromIt) {
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("t2");
  RunCmd("exec2");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteTwoTasksSelectFirstExecutionAndSearchItsOutput) {
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("t2");
  RunCmd("exec2");
  RunCmd("s\n(some|data)");
  EXPECT_OUT(HasSubstr(
      "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
      "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndViewGtestOutput) {
  ASSERT_INIT_CDT();
  RunCmd("t3");
  RunCmd("t3");
  RunCmd("exec2");
  RunCmd("g0");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("g99");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("g");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("g1");
  EXPECT_OUT(HasSubstr(OUT_LINKS_HIGHLIGHTED()));
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndSearchGtestOutput) {
  ASSERT_INIT_CDT();
  RunCmd("t3");
  RunCmd("t3");
  RunCmd("exec2");
  RunCmd("gs");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("gs0");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("gs99");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("gs1\n(some|data)");
  EXPECT_OUT(HasSubstr(
      "\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
      "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtest) {
  ProcessExec rerun;
  rerun.output_lines = CreateTestOutput(suite_rerun);
  mock.MockProc(cmd_test_rerun, rerun);
  ASSERT_INIT_CDT();
  RunCmd("t3");
  RunCmd("t3");
  RunCmd("exec2");
  RunCmd("gt");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("gt0");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("gt99");
  EXPECT_OUT(HasSubstrsInOrder(list_of_successful_tests));
  RunCmd("gt1");
  EXPECT_PROCS_EXACT(execs.kTests, execs.kTests, cmd_test_rerun);
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtestUntilItFails) {
  mock.MockProc(execs.kTests, exec_tests_successful);
  ProcessExec rerun_success;
  rerun_success.output_lines = CreateTestOutput(suite_rerun);
  suite_rerun[0].tests[0].is_failed = true;
  ProcessExec rerun_failure;
  rerun_failure.exit_code = 1;
  rerun_failure.output_lines = CreateTestOutput(suite_rerun);
  mock.MockProc(cmd_test_rerun, rerun_success);
  mock.MockProc(cmd_test_rerun, rerun_success);
  mock.MockProc(cmd_test_rerun, rerun_failure);
  ASSERT_INIT_CDT();
  RunCmd("t3");
  RunCmd("t3");
  RunCmd("exec2");
  RunCmd("gtr");
  EXPECT_OUT(HasSubstrsInOrder(list_of_failed_tests));
  RunCmd("gtr0");
  EXPECT_OUT(HasSubstrsInOrder(list_of_failed_tests));
  RunCmd("gtr99");
  EXPECT_OUT(HasSubstrsInOrder(list_of_failed_tests));
  RunCmd("gtr1");
  EXPECT_PROCS_EXACT(execs.kTests, execs.kTests, cmd_test_rerun, cmd_test_rerun,
                     cmd_test_rerun);
}

TEST_F(ExecTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtestWithDebugger) {
  std::string exec_debugger = WITH_DEBUG(cmd_test_rerun);
  mock.MockProc(execs.kTests, exec_tests_successful);
  mock.MockProc(exec_debugger);
  ASSERT_INIT_CDT();
  RunCmd("t3");
  RunCmd("t3");
  RunCmd("exec2");
  RunCmd("gd");
  EXPECT_OUT(HasSubstrsInOrder(list_of_failed_tests));
  RunCmd("gd0");
  EXPECT_OUT(HasSubstrsInOrder(list_of_failed_tests));
  RunCmd("gd99");
  EXPECT_OUT(HasSubstrsInOrder(list_of_failed_tests));
  RunCmd("gd1");
  EXPECT_PROCS_EXACT(execs.kTests, execs.kTests, exec_debugger);
}

TEST_F(ExecTest, StartExecuteTaskTwiceSelectFirstExecutionExecuteAnotherTaskSeeFirstTaskStillSelectedSelectLastExecutionExecuteAnotherTaskAndSeeItBeingSelectedAutomatically) {
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("t1");
  RunCmd("exec2");
  RunCmd("exec");
  EXPECT_OUT(ContainsRegex("-> 2 ..:00:01 \"task 1\""));
  EXPECT_OUT(HasSubstrNTimes("->", 1));
  RunCmd("t1");
  RunCmd("exec");
  EXPECT_OUT(ContainsRegex("-> 3 ..:00:01 \"task 1\""));
  EXPECT_OUT(HasSubstrNTimes("->", 1));
  RunCmd("exec1");
  RunCmd("t1");
  RunCmd("exec");
  EXPECT_OUT(ContainsRegex("-> 1 ..:00:04 \"task 1\""));
  EXPECT_OUT(HasSubstrNTimes("->", 1));
}

TEST_F(ExecTest, StartExecuteGtestTaskTwiceExecuteNormalTask110TimesWhileSelectingFirstNormalExecutionAndViewHistoryOf100ExecutionsThatIncludesOnlyOneLastGtestTaskAndSelectedFirstNormalExecution) {
  ASSERT_INIT_CDT();
  for (int i = 0; i < 2; i++) {
      RunCmd("t3");
  }
  for (int i = 0; i < 110; i++) {
      // Don't use other tasks that print file links - tests will take longer
      // to execute due to regex lookup of those links.
      RunCmd("t2");
      if (i == 1) {
          RunCmd("exec2");
      }
  }
  RunCmd("exec");
  // Last __gtest task execution does not get deleted
  EXPECT_OUT(ContainsRegex("100 ..:..:02 \"run tests\""));
  // Selected execution does not get deleted
  EXPECT_OUT(ContainsRegex("-> 99 ..:..:03 \"task 2\""));
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskAndSearchItsOutput) {
  ASSERT_INIT_CDT();
  RunCmd("t4");
  RunCmd("exec2");
  RunCmd("s\ntask");
  EXPECT_OUT(HasSubstr("\x1B[35m1:\x1B[0m\x1B[32mtask\x1B[0m 2"));
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskAndOpenLinkFromItsOutput) {
  ASSERT_INIT_CDT();
  RunCmd("t4");
  RunCmd("exec3");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteTaskWithPreTasksSelectPreTaskThenResetSelectionAndOpenLinksFromTheNewestTaskOutput) {
  ASSERT_INIT_CDT();
  RunCmd("t4");
  RunCmd("exec2");
  RunCmd("exec1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(ExecTest, StartExecuteGtestTaskWithPreTasksSelectOneOfPretasksWithLinksInOutputResetExecutionSelectionBackToTheLatestGtestTaskValidateThatThereAreNoLinksInItsOutput) {
  ASSERT_INIT_CDT();
  CMD("t5");
  CMD("exec2");
  CMD("exec1");
  CMD("o1");
  EXPECT_NOT_PROCS_LIKE(execs.kEditor);
}

TEST_F(ExecTest, StartExecuteTaskOpenLinksFromOuputValidateExecutionHistoryHasOnlyOneExecution) {
  ASSERT_INIT_CDT();
  CMD("t1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
  CMD("exec");
  // Execution history has only one line and its the first execution
  EXPECT_OUT(ContainsRegex("\n-> 1 ..:00:01 \"task 1\"\n"));
  EXPECT_OUT(HasSubstrNTimes("\n", 2));
}
