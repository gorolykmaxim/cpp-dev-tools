#include "test-lib.h"

class GsTest: public CdtTest {};

TEST_F(GsTest, StartAttemptToSearchGtestOutputWhenNoTestsHaveBeenExecutedYet) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("gs");
}

TEST_F(GsTest, StartExecuteGtestTaskWithPreTasksFailAttemptToSearchOutputOfTestThatDoesNotExistAndViewListOfFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gs");
  EXPECT_CMD("gs0");
  EXPECT_CMD("gs99");
}

TEST_F(GsTest, StartExecuteGtestTaskWithPreTasksFailAndSearchOutputOfTheTest) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gs1\n(C\\+\\+|with description|Failure)");
  EXPECT_CMD("gs2\n(C\\+\\+|with description|Failure)");
}

TEST_F(GsTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToSearchOutputOfTestThatDoesNotExistAndViewListOfAllTests) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gs");
  EXPECT_CMD("gs0");
  EXPECT_CMD("gs99");
}

TEST_F(GsTest, StartExecuteGtestTaskWithPreTasksSucceedAndSearchOutputOfOneOfTheTests) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gs1\n(some|data)");
  EXPECT_CMD("gs2\n(some|data)");
}
