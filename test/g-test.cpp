#include "test-lib.h"

class Gtest: public CdtTest {};

TEST_F(Gtest, StartAttemptToViewGtestTestsButSeeNoTestsHaveBeenExecutedYet) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("g");
}

TEST_F(Gtest, StartExecuteGtestTaskTryToViewGtestTestOutputWithIndexOutOfRangeAndSeeAllTestsList) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("g0");
  EXPECT_CMD("g99");
  EXPECT_CMD("g");
}

TEST_F(Gtest, StartExecuteGtestTaskViewGtestTaskOutputWithFileLinksHighlightedInTheOutputAndOpenLinks) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("g1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(Gtest, StartExecuteGtestTaskFailTryToViewGtestTestOutputWithIndexOutOfRangeAndSeeFailedTestsList) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("g0");
  EXPECT_CMD("g99");
  EXPECT_CMD("g");
}

TEST_F(Gtest, StartExecuteGtestTaskFailViewGtestTestOutputWithFileLinksHighlightedInTheOutputAndOpenLinks) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("g1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(Gtest, StartExecuteGtestTaskExecuteNonGtestTaskAndDisplayOutputOfOneOfTheTestsExecutedPreviously) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t8");
  EXPECT_CMD("t1");
  EXPECT_CMD("g1");
}
