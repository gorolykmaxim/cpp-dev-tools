#include "test-lib.h"
#include <gmock/gmock-matchers.h>

using namespace testing;

class Gtest: public CdtTest {};

TEST_F(Gtest, StartAttemptToViewGtestTestsButSeeNoTestsHaveBeenExecutedYet) {
  ASSERT_CDT_STARTED();
  CMD("g");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(Gtest, StartExecuteGtestTaskTryToViewGtestTestOutputWithIndexOutOfRangeAndSeeAllTestsList) {
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("g0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("g99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("g");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
}

TEST_F(Gtest, StartExecuteGtestTaskViewGtestTaskOutputWithFileLinksHighlightedInTheOutputAndOpenLinks) {
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("g1");
  EXPECT_OUTPUT_LINKS_TO_OPEN_2();
}

TEST_F(Gtest, StartExecuteGtestTaskFailTryToViewGtestTestOutputWithIndexOutOfRangeAndSeeFailedTestsList) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("g0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("g99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("g");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
}

TEST_F(Gtest, StartExecuteGtestTaskFailViewGtestTestOutputWithFileLinksHighlightedInTheOutputAndOpenLinks) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("g1");
  EXPECT_OUTPUT_LINKS_TO_OPEN_2();
}

TEST_F(Gtest, StartExecuteGtestTaskExecuteNonGtestTaskAndDisplayOutputOfOneOfTheTestsExecutedPreviously) {
  ASSERT_CDT_STARTED();
  CMD("t8");
  CMD("t1");
  CMD("g1");
  EXPECT_OUT(HasSubstr(OUT_LINKS_HIGHLIGHTED()));
}
