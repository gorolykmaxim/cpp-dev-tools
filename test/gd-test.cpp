#include "test-lib.h"

class GdTest: public CdtTest {};

TEST_F(GdTest, StartAttemptToRerunGtestWithDebuggerWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
  mock.MockReadFile(paths.kUserConfig);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("gd");
}

TEST_F(GdTest, StartAttemptToRerunGtestWithDebuggerWhenNoTestsHaveBeenExecutedYet) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("gd");
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksFailAttemptToRerunTestThatDoesNotExistWithDebuggerAndViewListOfFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gd");
  EXPECT_CMD("gd0");
  EXPECT_CMD("gd99");
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksFailAndRerunFailedTestWithDebugger) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  testing::InSequence seq;
  EXPECT_PROC(WITH_DEBUG("tests --gtest_filter='failed_test_suit_1.test1'"));
  EXPECT_PROC(WITH_DEBUG("tests --gtest_filter='failed_test_suit_2.test1'"));
  EXPECT_CMD("gd1");
  EXPECT_CMD("gd2");
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRerunTestThatDoesNotExistWithDebuggerAndViewListOfAllTests) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  EXPECT_CMD("gd");
  EXPECT_CMD("gd0");
  EXPECT_CMD("gd99");
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksSucceedAndRerunOneOfTestsWithDebugger) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t10");
  testing::InSequence seq;
  EXPECT_PROC(WITH_DEBUG("tests --gtest_filter='test_suit_1.test1'"));
  EXPECT_PROC(WITH_DEBUG("tests --gtest_filter='test_suit_1.test2'"));
  EXPECT_CMD("gd1");
  EXPECT_CMD("gd2");
}
