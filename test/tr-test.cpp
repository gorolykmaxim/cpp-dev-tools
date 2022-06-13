#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class TrTest: public CdtTest {};

TEST_F(TrTest, StartRepeatedlyExecuteTaskUntilItFails) {
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  ASSERT_CDT_STARTED();
  CMD("tr8");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
      TASK_COMPLETE("run tests"), TASK_COMPLETE("run tests"),
      TASK_FAILED("run tests", failed_gtest_exec.exit_code)}));
  EXPECT_PROCS_EXACT(execs.kTests, execs.kTests, execs.kTests);
}

TEST_F(TrTest, StartRepeatedlyExecuteTaskUntilOneOfItsPreTasksFails) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("tr9");
  EXPECT_OUT(HasSubstr(TASK_FAILED("run tests", failed_gtest_exec.exit_code)));
  EXPECT_PROCS_EXACT(execs.kTests);
}

TEST_F(TrTest, StartRepeatedlyExecuteLongTaskAndAbortIt) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().is_long = true;
  ASSERT_CDT_STARTED();
  INTERRUPT_CMD("tr1");
  EXPECT_OUT(HasSubstrsInOrder(std::vector<std::string>{
    "hello world!", TASK_FAILED("hello world!", -1)}));
  EXPECT_PROCS_EXACT(execs.kHelloWorld);
}
