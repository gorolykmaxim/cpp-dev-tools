#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class GtTest: public CdtTest {
protected:
  ProcessExec exec_tests_fail, exec_tests_success;

  void SetUp() override {
    tasks = {
        CreateTaskAndProcess("pre task"),
        CreateTask("run tests", "__gtest " + execs.kTests, {"pre task"})};
    std::vector<DummyTestSuite> suites = {
        DummyTestSuite{"suite", {
            DummyTest{"test1", {OUT_LINKS_NOT_HIGHLIGHTED()}},
            DummyTest{"test2"}}}};
    exec_tests_success.output_lines = CreateTestOutput(suites);
    suites[0].tests[0].is_failed = true;
    suites[0].tests[1].is_failed = true;
    exec_tests_fail.exit_code = 1;
    exec_tests_fail.output_lines = CreateTestOutput(suites);
    CdtTest::SetUp();
  }
};

TEST_F(GtTest, StartAttemptToRerunGtTestWhenNoTestsHaveBeenExecutedYet) {
  ASSERT_INIT_CDT();
  RunCmd("gt");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
  // The command should repeat on enter
  RunCmd("");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksFailAttemptToRerunTestThatDoesNotExistAndViewListOfFailedTests) {
  std::vector<std::string> failed_tests = {"1 \"suite.test1\"",
                                           "2 \"suite.test2\""};
  mock.MockProc(execs.kTests, exec_tests_fail);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("gt");
  EXPECT_OUT(HasSubstrsInOrder(failed_tests));
  RunCmd("gt0");
  EXPECT_OUT(HasSubstrsInOrder(failed_tests));
  RunCmd("gt99");
  EXPECT_OUT(HasSubstrsInOrder(failed_tests));
  // Check that we don't attempt to rerun any test
  EXPECT_PROCS_EXACT(tasks[0]["command"], execs.kTests);
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksFailAndRerunFailedTest) {
  ProcessExec rerun1;
  rerun1.exit_code = 1;
  ProcessExec rerun2 = rerun1;
  rerun1.output_lines = CreateTestOutput({
      DummyTestSuite{"suite", {DummyTest{"test1", {OUT_LINKS_NOT_HIGHLIGHTED(),
                                                   OUT_TEST_ERROR()},
                                          true}}}});
  rerun2.output_lines = CreateTestOutput({
      DummyTestSuite{"suite", {DummyTest{"test2", {OUT_TEST_ERROR()}, true}}}});
  std::string cmd_rerun1 = execs.kTests + WITH_GT_FILTER("suite.test1");
  std::string cmd_rerun2 = execs.kTests + WITH_GT_FILTER("suite.test2");
  mock.MockProc(execs.kTests, exec_tests_fail);
  mock.MockProc(cmd_rerun1, rerun1);
  mock.MockProc(cmd_rerun2, rerun2);
  std::vector<std::string> out_rerun1 = {OUT_LINKS_HIGHLIGHTED(),
                                         TASK_FAILED("suite.test1",
                                                     rerun1.exit_code)};
  std::vector<std::string> out_rerun2 = {TASK_FAILED("suite.test2",
                                                     rerun2.exit_code)};
  std::string cmd_pre_task = tasks[0]["command"];
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("gt1");
  EXPECT_OUT(HasSubstrsInOrder(out_rerun1));
  RunCmd("gt2");
  EXPECT_OUT(HasSubstrsInOrder(out_rerun2));
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests, cmd_pre_task, cmd_rerun1,
                     cmd_pre_task, cmd_rerun2);
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksSucceedAttemptToRerunTestThatDoesNotExistAndViewListOfAllTests) {
  std::vector<std::string> successful_tests = {"1 \"suite.test1\"",
                                               "2 \"suite.test2\""};
  mock.MockProc(execs.kTests, exec_tests_success);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("gt");
  EXPECT_OUT(HasSubstrsInOrder(successful_tests));
  RunCmd("gt0");
  EXPECT_OUT(HasSubstrsInOrder(successful_tests));
  RunCmd("gt99");
  EXPECT_OUT(HasSubstrsInOrder(successful_tests));
  // Check that we don't attempt to rerun any test
  EXPECT_PROCS_EXACT(tasks[0]["command"], execs.kTests);
}

TEST_F(GtTest, StartExecuteGtTestTaskWithPreTasksSucceedAndRerunOneOfTest) {
  ProcessExec rerun1, rerun2;
  rerun1.output_lines = CreateTestOutput({
      DummyTestSuite{"suite", {
          DummyTest{"test1", {OUT_LINKS_NOT_HIGHLIGHTED()}}}}});
  rerun2.output_lines = CreateTestOutput({
      DummyTestSuite{"suite", {DummyTest{"test2"}}}});
  std::string cmd_rerun1 = execs.kTests + WITH_GT_FILTER("suite.test1");
  std::string cmd_rerun2 = execs.kTests + WITH_GT_FILTER("suite.test2");
  std::string cmd_pre_task = tasks[0]["command"];
  mock.MockProc(execs.kTests, exec_tests_success);
  mock.MockProc(cmd_rerun1, rerun1);
  mock.MockProc(cmd_rerun2, rerun2);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("gt1");
  EXPECT_OUT(HasSubstr(OUT_LINKS_HIGHLIGHTED()));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("suite.test1")));
  RunCmd("gt2");
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("suite.test2")));
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests, cmd_pre_task, cmd_rerun1,
                     cmd_pre_task, cmd_rerun2);
}
