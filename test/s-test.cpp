#include "test-lib.h"

class STest: public CdtTest {};

TEST_F(STest, StartAttemptToSearchExecutionOutputButFailDueToNoTaskBeingExecutedBeforeIt) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("s");
}

TEST_F(STest, StartExecuteTaskAttemptToSearchItsOutputWithInvalidRegexAndFail) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("s\n[");
}

TEST_F(STest, StartExecuteTaskSearchItsOutputAndFindNoResults) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("s\nnon\\-existent");
}

TEST_F(STest, StartExecuteTaskSearchItsOutputAndFindResults) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("s\n(some|data)");
}

TEST_F(STest, StartExecuteGtestTaskSearchItsRawOutput) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("s\n(some|data)");
}
