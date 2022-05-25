#include "test-lib.h"

class TrTest: public CdtTest {};

TEST_F(TrTest, StartRepeatedlyExecuteTaskUntilItFails) {
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("tr8");
}

TEST_F(TrTest, StartRepeatedlyExecuteTaskUntilOneOfItsPreTasksFails) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("tr9");
}

TEST_F(TrTest, StartRepeatedlyExecuteLongTaskAndAbortIt) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().is_long = true;
  EXPECT_CDT_STARTED();
  EXPECT_INTERRUPTED_CMD("tr1");
}
