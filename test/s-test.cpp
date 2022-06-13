#include "test-lib.h"
#include <gmock/gmock-matchers.h>

using namespace testing;

class STest: public CdtTest {};

TEST_F(STest, StartAttemptToSearchExecutionOutputButFailDueToNoTaskBeingExecutedBeforeIt) {
  ASSERT_CDT_STARTED();
  CMD("s");
  EXPECT_OUT(HasSubstr("No task has been executed yet"));
}

TEST_F(STest, StartExecuteTaskAttemptToSearchItsOutputWithInvalidRegexAndFail) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  ASSERT_CDT_STARTED();
  CMD("t1");
  CMD("s\n[");
  EXPECT_OUT(HasSubstr("Invalid regular expression"));
}

TEST_F(STest, StartExecuteTaskSearchItsOutputAndFindNoResults) {
  ASSERT_CDT_STARTED();
  CMD("t1");
  CMD("s\nnon\\-existent");
  EXPECT_OUT(HasSubstr("No matches found"));
}

TEST_F(STest, StartExecuteTaskSearchItsOutputAndFindResults) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  ASSERT_CDT_STARTED();
  CMD("t1");
  CMD("s\n(some|data)");
  EXPECT_OUT(HasSubstr(
      "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
      "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
}

TEST_F(STest, StartExecuteGtestTaskSearchItsRawOutput) {
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("s\n(some|data)");
  EXPECT_OUT(HasSubstr(
    "\x1B[35m7:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
    "\x1B[35m8:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
}
