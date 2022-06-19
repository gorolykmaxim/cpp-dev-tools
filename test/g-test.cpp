#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class Gtest: public CdtTest {
protected:
  ProcessExec exec_test_success, exec_test_failure;

  void SetUp() override {
    tasks = {
      CreateTask("run tests", "__gtest " + execs.kTests),
      CreateTaskAndProcess("primary task")
    };
    std::vector<DummyTestSuite> suites = {
        DummyTestSuite{"suite1", {
            DummyTest{"test1", {OUT_LINKS_NOT_HIGHLIGHTED()}},
            DummyTest{"test2"}}}};
    exec_test_success.output_lines = CreateTestOutput(suites);
    suites[0].tests[0].is_failed = true;
    exec_test_failure.output_lines = CreateTestOutput(suites);
    exec_test_failure.exit_code = 1;
    Init();
  }
};

TEST_F(Gtest, StartAttemptToViewGtestTestsButSeeNoTestsHaveBeenExecutedYet) {
  ASSERT_INIT_CDT();
  RunCmd("g");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(Gtest, StartExecuteGtestTaskTryToViewGtestTestOutputWithIndexOutOfRangeAndSeeAllTestsList) {
  std::vector<std::string> test_names = {"1 \"suite1.test1\"",
                                         "2 \"suite1.test2\""};
  mock.MockProc(execs.kTests, exec_test_success);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("g0");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("g99");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("g");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
}

TEST_F(Gtest, StartExecuteGtestTaskViewGtestTaskOutputWithFileLinksHighlightedInTheOutputAndOpenLinks) {
  mock.MockProc(execs.kTests, exec_test_success);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("g1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(Gtest, StartExecuteGtestTaskFailTryToViewGtestTestOutputWithIndexOutOfRangeAndSeeFailedTestsList) {
  std::vector<std::string> test_names = {"1 \"suite1.test1\""};
  mock.MockProc(execs.kTests, exec_test_failure);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("g0");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("g99");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("g");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
}

TEST_F(Gtest, StartExecuteGtestTaskFailViewGtestTestOutputWithFileLinksHighlightedInTheOutputAndOpenLinks) {
  mock.MockProc(execs.kTests, exec_test_failure);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("g1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(Gtest, StartExecuteGtestTaskExecuteNonGtestTaskAndDisplayOutputOfOneOfTheTestsExecutedPreviously) {
  mock.MockProc(execs.kTests, exec_test_success);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("t2");
  RunCmd("g1");
  EXPECT_OUT(HasSubstr(OUT_LINKS_HIGHLIGHTED()));
}
