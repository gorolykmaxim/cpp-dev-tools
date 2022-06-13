#include "test-lib.h"
#include <gmock/gmock-matchers.h>

using namespace testing;

class GsTest: public CdtTest {};

TEST_F(GsTest, StartAttemptToSearchGtestOutputWhenNoTestsHaveBeenExecutedYet) {
  ASSERT_CDT_STARTED();
  CMD("gs");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(GsTest, StartExecuteGtestTaskWithPreTasksFailAttemptToSearchOutputOfTestThatDoesNotExistAndViewListOfFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gs");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gs0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gs99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
}

TEST_F(GsTest, StartExecuteGtestTaskWithPreTasksFailAndSearchOutputOfTheTest) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gs1\n(C\\+\\+|with description|Failure)");
  EXPECT_OUT(HasSubstr(
      "\x1B[35m5:\x1B[0munknown file: \x1B[32mFailure\x1B[0m\n"
      "\x1B[35m6:\x1B[0m\x1B[32mC++\x1B[0m exception "
      "\x1B[32mwith description\x1B[0m \"\" thrown in the test body.\n"));
  CMD("gs2\n(C\\+\\+|with description|Failure)");
  EXPECT_OUT(HasSubstr(
      "\x1B[35m2:\x1B[0munknown file: \x1B[32mFailure\x1B[0m\n"
      "\x1B[35m3:\x1B[0m\x1B[32mC++\x1B[0m exception "
      "\x1B[32mwith description\x1B[0m \"\" thrown in the test body.\n"));
}

TEST_F(GsTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToSearchOutputOfTestThatDoesNotExistAndViewListOfAllTests) {
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gs");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gs0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gs99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
}

TEST_F(GsTest, StartExecuteGtestTaskWithPreTasksSucceedAndSearchOutputOfOneOfTheTests) {
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gs1\n(some|data)");
  EXPECT_OUT(HasSubstr(
      "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
      "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"));
  CMD("gs2\n(some|data)");
  EXPECT_OUT(HasSubstr("No matches found"));
}
