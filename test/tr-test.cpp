#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class TrTest: public CdtTest {
protected:
  ProcessExec exec_tests_fail, exec_tests_success;

  void SetUp() override {
    tasks = {CreateTask("run tests", "__gtest " + execs.kTests)};
    std::vector<DummyTestSuite> suites = {
        DummyTestSuite{"suite", {DummyTest{"test"}}}};
    exec_tests_success.output_lines = CreateTestOutput(suites);
    suites[0].tests[0].is_failed = true;
    exec_tests_fail.exit_code = 1;
    exec_tests_fail.output_lines = CreateTestOutput(suites);
    CdtTest::SetUp();
  }
};

TEST_F(TrTest, StartRepeatedlyExecuteTaskUntilItFails) {
  mock.MockProc(execs.kTests, exec_tests_success);
  mock.MockProc(execs.kTests, exec_tests_success);
  mock.MockProc(execs.kTests, exec_tests_fail);
  ASSERT_INIT_CDT();
  RunCmd("tr1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      TASK_COMPLETE("run tests"), TASK_COMPLETE("run tests"),
      TASK_FAILED("run tests", exec_tests_fail.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests, execs.kTests, execs.kTests);
}

TEST_F(TrTest, StartRepeatedlyExecuteTaskUntilOneOfItsPreTasksFails) {
  tasks.push_back(CreateTaskAndProcess("primary task", {"run tests"}));
  mock.MockProc(execs.kTests, exec_tests_fail);
  MockTasksConfig();
  ASSERT_INIT_CDT();
  RunCmd("tr2");
  EXPECT_OUT(HasSubstr(TASK_FAILED("run tests", exec_tests_fail.exit_code)));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TrTest, StartRepeatedlyExecuteLongTaskAndAbortIt) {
  std::string output = "primary task";
  std::string cmd = "echo " + output;
  tasks = {CreateTask(output, cmd)};
  ProcessExec exec;
  exec.is_long = true;
  exec.output_lines = {output};
  mock.MockProc(cmd, exec);
  MockTasksConfig();
  ASSERT_INIT_CDT();
  InterruptCmd("tr1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      output, TASK_FAILED("primary task", -1)}));
  EXPECT_PROCS_EXACT(cmd);
}
