#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class GdTest: public CdtTest {
protected:
  std::vector<nlohmann::json> tasks;
  std::vector<std::string> test_names;
  ProcessExec exec_tests_failed, exec_tests_successful;
  std::string cmd_pre_task, cmd_test1_rerun, cmd_test2_rerun;

  void SetUp() override {
    tasks = {
        CreateTaskAndProcess("pre task"),
        CreateTask("run tests", "__gtest " + execs.kTests, {"pre task"})};
    std::vector<DummyTestSuite> suites = {
        DummyTestSuite{"suite", {
            DummyTest{"test1"},
            DummyTest{"test2"}}}};
    exec_tests_successful.output_lines = CreateTestOutput(suites);
    suites[0].tests[0].is_failed = true;
    suites[0].tests[1].is_failed = true;
    exec_tests_failed.exit_code = 1;
    exec_tests_failed.output_lines = CreateTestOutput(suites);
    test_names = {"1 \"suite.test1\"", "2 \"suite.test2\""};
    cmd_pre_task = tasks[0]["command"];
    cmd_test1_rerun = WITH_DEBUG("tests" WITH_GT_FILTER("suite.test1"));
    cmd_test2_rerun = WITH_DEBUG("tests" WITH_GT_FILTER("suite.test2"));
    mock.MockProc(cmd_test1_rerun);
    mock.MockProc(cmd_test2_rerun);
    Init();
  }
};

TEST_F(GdTest, StartAttemptToRerunGtestWithDebuggerWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
  mock.MockReadFile(paths.kUserConfig);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("gd");
  EXPECT_OUT(HasSubstr("'debug_command' is not specified"));
  EXPECT_OUT(HasPath(paths.kUserConfig));
}

TEST_F(GdTest, StartAttemptToRerunGtestWithDebuggerWhenNoTestsHaveBeenExecutedYet) {
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("gd");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksFailAttemptToRerunTestThatDoesNotExistWithDebuggerAndViewListOfFailedTests) {
  mock.MockProc(execs.kTests, exec_tests_failed);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("t2");
  RunCmd("gd");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("gd0");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("gd99");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  // Check that we don't attempt to execute any debugger calls
  EXPECT_PROCS_EXACT("echo pre task", execs.kTests);
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksFailAndRerunFailedTestWithDebugger) {
  mock.MockProc(execs.kTests, exec_tests_failed);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("t2");
  RunCmd("gd1");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("suite.test1")));
  RunCmd("gd2");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("suite.test2")));
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests, cmd_pre_task, cmd_test1_rerun,
                     cmd_pre_task, cmd_test2_rerun);
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRerunTestThatDoesNotExistWithDebuggerAndViewListOfAllTests) {
  mock.MockProc(execs.kTests, exec_tests_successful);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("t2");
  RunCmd("gd");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("gd0");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("gd99");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  // Check that we don't attempt to execute any debugger calls
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests);
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksSucceedAndRerunOneOfTestsWithDebugger) {
  mock.MockProc(execs.kTests, exec_tests_successful);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("t2");
  RunCmd("gd1");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("suite.test1")));
  RunCmd("gd2");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("suite.test2")));
  EXPECT_PROCS_EXACT(cmd_pre_task, execs.kTests, cmd_pre_task, cmd_test1_rerun,
                     cmd_pre_task, cmd_test2_rerun);
}
