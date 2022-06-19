#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class GtrTest: public CdtTest {
protected:
  ProcessExec exec_tests_success, exec_tests_fail;
  ProcessExec exec_rerun_success, exec_rerun_fail;
  std::string cmd_pre_task;
  std::string cmd_rerun;
  std::vector<std::string> test_names_success;

  void SetUp() override {
    tasks = {
        CreateTaskAndProcess("pre task"),
        CreateTask("run tests", "__gtest " + execs.kTests, {"pre task"})};
    std::vector<DummyTestSuite> suites = {
        DummyTestSuite{"suite", {
            DummyTest{"test1", {OUT_LINKS_NOT_HIGHLIGHTED()}},
            DummyTest{"test2"}}}};
    exec_tests_success.output_lines = CreateTestOutput(suites);
    suites[0].tests[0].output_lines = {OUT_LINKS_NOT_HIGHLIGHTED(),
                                       OUT_TEST_ERROR()};
    suites[0].tests[0].is_failed = true;
    exec_tests_fail.exit_code = 1;
    exec_tests_fail.output_lines = CreateTestOutput(suites);
    std::vector<DummyTestSuite> suites_rerun = {
        DummyTestSuite{"suite", {
            DummyTest{"test1", {OUT_LINKS_NOT_HIGHLIGHTED()}}}}};
    exec_rerun_success.output_lines = CreateTestOutput(suites_rerun);
    suites_rerun[0].tests[0].is_failed = true;
    suites_rerun[0].tests[0].output_lines = {OUT_LINKS_NOT_HIGHLIGHTED(),
                                             OUT_TEST_ERROR()};
    exec_rerun_fail.exit_code = 1;
    exec_rerun_fail.output_lines = CreateTestOutput(suites_rerun);
    cmd_pre_task = tasks[0]["command"];
    cmd_rerun = execs.kTests + WITH_GT_FILTER("suite.test1");
    test_names_success = {"1 \"suite.test1\"", "2 \"suite.test2\""};
    CdtTest::SetUp();
  }
};

TEST_F(GtrTest, StartRepeatedlyExecuteGtestTaskWithPreTasksUntilItFailsAndRepeatedlyRerunFailedTestUntilItFails) {
  mock.MockProc(execs.kTests, exec_tests_success);
  mock.MockProc(execs.kTests, exec_tests_success);
  mock.MockProc(execs.kTests, exec_tests_fail);
  mock.MockProc(cmd_rerun, exec_rerun_success);
  mock.MockProc(cmd_rerun, exec_rerun_success);
  mock.MockProc(cmd_rerun, exec_rerun_fail);
  std::string out_links = OUT_LINKS_HIGHLIGHTED();
  std::string task_complete = TASK_COMPLETE("suite.test1");
  std::vector<std::string> expected_out = {
      out_links, task_complete, out_links, task_complete, out_links,
      OUT_TEST_ERROR(), TASK_FAILED("suite.test1", exec_rerun_fail.exit_code)};
  ASSERT_INIT_CDT();
  RunCmd("tr2");
  RunCmd("gtr1");
  EXPECT_OUT(HasSubstrsInOrder(expected_out));
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests, execs.kTests, execs.kTests,
                     cmd_pre_task, cmd_rerun, cmd_rerun, cmd_rerun);
}

TEST_F(GtrTest, StartExecuteGtestTaskWithPreTasksSucceedRepeatedlyRerunOneOfTheTestsAndFailDueToFailedPreTask) {
  ProcessExec exec_pre_task_fail;
  exec_pre_task_fail.exit_code = 1;
  mock.MockProc(cmd_pre_task, exec_pre_task_fail);
  mock.MockProc(execs.kTests, exec_tests_success);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("gtr1");
  EXPECT_OUT(HasSubstr(TASK_FAILED("pre task", exec_pre_task_fail.exit_code)));
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests, cmd_pre_task);
}

TEST_F(GtrTest, StartExecuteLongGtestTaskWithPreTasksAbortItRepeatedlyRerunFailedTestAndAbortItAgain) {
  exec_tests_success.is_long = true;
  exec_tests_success.output_lines = CreateAbortedTestOutput("suite", "test1");
  ProcessExec rerun;
  rerun.exit_code = 1;
  rerun.is_long = true;
  rerun.output_lines = exec_tests_success.output_lines;
  mock.MockProc(cmd_rerun, rerun);
  mock.MockProc(execs.kTests, exec_tests_success);
  ASSERT_INIT_CDT();
  InterruptCmd("t2");
  InterruptCmd("gtr1");
  EXPECT_OUT(HasSubstr(TASK_FAILED("suite.test1", -1)));
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests, cmd_pre_task, cmd_rerun);
}

TEST_F(GtrTest, StartAttemptToRepeatedlyRerunGtestWhenNoTestsHaveBeenExecutedYet) {
  ASSERT_INIT_CDT();
  RunCmd("gtr");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(GtrTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRepeatedlyRerunTestThatDoesNotExistAndViewListOfAllTests) {
  mock.MockProc(execs.kTests, exec_tests_success);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("gtr");
  EXPECT_OUT(HasSubstrsInOrder(test_names_success));
  RunCmd("gtr0");
  EXPECT_OUT(HasSubstrsInOrder(test_names_success));
  RunCmd("gtr99");
  EXPECT_OUT(HasSubstrsInOrder(test_names_success));
  // Check that we don't attempt to rerun any test
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests);
}

TEST_F(GtrTest, StartExecuteGtestTaskRerunOneOfItsTestsAndDisplayListOfOriginallyExecutedTests) {
  mock.MockProc(execs.kTests, exec_tests_success);
  mock.MockProc(cmd_rerun, exec_rerun_success);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("gt1");
  RunCmd("g");
  EXPECT_OUT(HasSubstrsInOrder(test_names_success));
}
