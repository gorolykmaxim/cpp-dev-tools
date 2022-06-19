#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class TGtestTest: public CdtTest {
protected:
  ProcessExec exec_not_gtest, exec_test_aborted, exec_tests_success,
              exec_tests_fail, exec_test_fail;
  std::string cmd_primary;
  std::vector<std::string> test_names_failed;

  void SetUp() override {
    tasks = {
        CreateTask("run tests", "__gtest " + execs.kTests),
        CreateTaskAndProcess("primary task", {"run tests"})};
    exec_not_gtest.output_lines = {"hello world!"};
    exec_test_aborted.output_lines = CreateAbortedTestOutput("suite", "test");
    cmd_primary = tasks[1]["command"];
    std::vector<DummyTestSuite> suites = {
        DummyTestSuite{"suite1", {
            DummyTest{"test1"},
            DummyTest{"test2"}}},
        DummyTestSuite{"suite2", {
            DummyTest{"test1"}}}};
    exec_tests_success.output_lines = CreateTestOutput(suites);
    suites[0].tests[1].is_failed = true;
    suites[1].tests[0].is_failed = true;
    exec_tests_fail.output_lines = CreateTestOutput(suites);
    exec_tests_fail.exit_code = 1;
    test_names_failed = {"1 \"suite1.test2\"", "2 \"suite2.test1\""};
    suites = {
        DummyTestSuite{"suite", {
            DummyTest{"test1"},
            DummyTest{"test2", {OUT_LINKS_NOT_HIGHLIGHTED(), OUT_TEST_ERROR()},
                      true}}}};
    exec_test_fail.exit_code = 1;
    exec_test_fail.output_lines = CreateTestOutput(suites);
    CdtTest::SetUp();
  }
};

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithNoTests) {
  ProcessExec exec;
  exec.output_lines = CreateTestOutput({});
  mock.MockProc(execs.kTests, exec);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run tests")));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAttemptToExecuteGtestTaskWithNonExistentBinaryAndFail) {
  ProcessExec exec;
  exec.exit_code = 127;
  exec.output_lines = {execs.kTests + " does not exist"};
  exec.stderr_lines = {0};
  mock.MockProc(execs.kTests, exec);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "'" + execs.kTests + "' is not a google test executable",
      TASK_FAILED("run tests", exec.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithNonGtestBinary) {
  mock.MockProc(execs.kTests, exec_not_gtest);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "'" + execs.kTests + "' is not a google test executable",
      TASK_FAILED("run tests", exec_not_gtest.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithNonGtestBinaryThatDoesNotFinishAndAbortItManually) {
  exec_not_gtest.is_long = true;
  mock.MockProc(execs.kTests, exec_not_gtest);
  ASSERT_INIT_CDT();
  InterruptCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "'" + execs.kTests + "' is not a google test executable",
      TASK_FAILED("run tests", -1)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskThatExitsWith0ExitCodeInTheMiddle) {
  mock.MockProc(execs.kTests, exec_test_aborted);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      TASK_FAILED("run tests", 0)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskThatExitsWith1ExitCodeInTheMiddle) {
  exec_test_aborted.exit_code = 1;
  mock.MockProc(execs.kTests, exec_test_aborted);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      TASK_FAILED("run tests", exec_test_aborted.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAttemptToExecuteTaskWithGtestPreTaskThatExitsWith0ExitCodeInTheMiddleAndFail) {
  mock.MockProc(execs.kTests, exec_test_aborted);
  mock.MockProc(cmd_primary);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      TASK_FAILED("run tests", exec_test_aborted.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithMultipleSuites) {
  mock.MockProc(execs.kTests, exec_tests_success);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run tests")));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteGtestTaskWithMultipleSuitesEachHavingFailedTests) {
  mock.MockProc(execs.kTests, exec_tests_fail);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(test_names_failed));
  EXPECT_OUT(HasSubstr(TASK_FAILED("run tests", exec_tests_fail.exit_code)));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartAndExecuteTaskWithGtestPreTaskWithMultipleSuites) {
  mock.MockProc(execs.kTests, exec_tests_success);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "primary task",
      TASK_COMPLETE("primary task")}));
  EXPECT_PROCS_EXACT(execs.kTests, cmd_primary);
}

TEST_F(TGtestTest, StartAndExecuteTaskWithGtestPreTaskWithMultipleSuitesEachHavingFailedTests) {
  mock.MockProc(execs.kTests, exec_tests_fail);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUT(HasSubstrsInOrder(test_names_failed));
  EXPECT_OUT(HasSubstr(TASK_FAILED("run tests", exec_tests_fail.exit_code)));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithLongTestAndAbortIt) {
  exec_test_aborted.is_long = true;
  mock.MockProc(execs.kTests, exec_test_aborted);
  ASSERT_INIT_CDT();
  InterruptCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      TASK_FAILED("run tests", -1)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteGtestTaskWithFailedTestsAndLongTestAndAbortIt) {
  exec_test_aborted.exit_code = 1;
  exec_test_aborted.is_long = true;
  exec_test_aborted.output_lines.push_back("[  FAILED  ] suite.test (0 ms)");
  exec_test_aborted.output_lines.push_back("[ RUN      ] suite.long_test");
  mock.MockProc(execs.kTests, exec_test_aborted);
  ASSERT_INIT_CDT();
  InterruptCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      "Tests have finished prematurely",
      "1 \"suite.test\"",
      "2 \"suite.long_test\"",
      TASK_FAILED("run tests", -1)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteGtestTaskFailOneOfTheTestsAndViewAutomaticallyDisplayedTestOutput) {
  mock.MockProc(execs.kTests, exec_test_fail);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      OUT_LINKS_HIGHLIGHTED(), OUT_TEST_ERROR(),
      TASK_FAILED("run tests", exec_test_fail.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TGtestTest, StartExecuteTaskWithGtestPreTaskFailOneOfTheTestsAndViewAutomaticallyDisplayedTestOutput) {
  mock.MockProc(execs.kTests, exec_test_fail);
  mock.MockProc(cmd_primary);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      OUT_LINKS_HIGHLIGHTED(), OUT_TEST_ERROR(),
      TASK_FAILED("run tests", exec_test_fail.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests);
}
