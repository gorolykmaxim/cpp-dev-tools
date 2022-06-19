#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <vector>

using namespace testing;

class STest: public CdtTest {
protected:
  void SetUp() override {
    tasks = {
        CreateTask("primary task", "echo primary task")};
    ProcessExec exec;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    mock.MockProc(tasks[0]["command"], exec);
    Init();
  }
};

TEST_F(STest, StartAttemptToSearchExecutionOutputButFailDueToNoTaskBeingExecutedBeforeIt) {
  ASSERT_INIT_CDT();
  RunCmd("s");
  EXPECT_OUT(HasSubstr("No task has been executed yet"));
}

TEST_F(STest, StartExecuteTaskAttemptToSearchItsOutputWithInvalidRegexAndFail) {
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("s\n[");
  EXPECT_OUT(HasSubstr("Invalid regular expression"));
}

TEST_F(STest, StartExecuteTaskSearchItsOutputAndFindNoResults) {
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("s\nnon\\-existent");
  EXPECT_OUT(HasSubstr("No matches found"));
}

TEST_F(STest, StartExecuteTaskSearchItsOutputAndFindResults) {
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("s\n(some|data)");
  EXPECT_OUT(HasSubstr(
      "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
      "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
}

TEST_F(STest, StartExecuteGtestTaskSearchItsRawOutput) {
  std::vector<DummyTestSuite> suites = {
      DummyTestSuite{"suite", {
          DummyTest{"test", {OUT_LINKS_NOT_HIGHLIGHTED()}}}}};
  ProcessExec exec;
  exec.output_lines = CreateTestOutput(suites);
  mock.MockProc(execs.kTests, exec);
  tasks.push_back(CreateTask("run tests", "__gtest " + execs.kTests));
  MockTasksConfig();
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("s\n(some|data)");
  EXPECT_OUT(HasSubstr(
    "\x1B[35m7:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
    "\x1B[35m8:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
}
