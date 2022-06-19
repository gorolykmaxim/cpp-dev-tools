#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class GsTest: public CdtTest {
protected:
  ProcessExec exec_tests_failed, exec_tests_successful;
  std::vector<std::string> test_names;

  void SetUp() override {
    tasks = {CreateTask("run tests", "__gtest " + execs.kTests)};
    std::vector<DummyTestSuite> suites = {
        DummyTestSuite{"suite", {
            DummyTest{"test1", {"some random", "data"}},
            DummyTest{"test2", {"another", "random output"}}}}};
    exec_tests_successful.output_lines = CreateTestOutput(suites);
    suites[0].tests[0].is_failed = true;
    suites[0].tests[1].is_failed = true;
    exec_tests_failed.exit_code = 1;
    exec_tests_failed.output_lines = CreateTestOutput(suites);
    test_names = {"1 \"suite.test1\"", "2 \"suite.test2\""};
    Init();
  }
};

TEST_F(GsTest, StartAttemptToSearchGtestOutputWhenNoTestsHaveBeenExecutedYet) {
  ASSERT_INIT_CDT();
  RunCmd("gs");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(GsTest, StartExecuteGtestTaskFailAttemptToSearchOutputOfTestThatDoesNotExistAndViewListOfFailedTests) {
  mock.MockProc(execs.kTests, exec_tests_failed);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("gs");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("gs0");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("gs99");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
}

TEST_F(GsTest, StartExecuteGtestTaskFailAndSearchOutputOfTheTest) {
  mock.MockProc(execs.kTests, exec_tests_failed);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("gs1\n(some|data)");
  EXPECT_OUT(HasSubstr("\x1B[35m1:\x1B[0m\x1B[32msome\x1B[0m random\n"
                       "\x1B[35m2:\x1B[0m\x1B[32mdata\x1B[0m\n"));
  RunCmd("gs2\nrandom");
  EXPECT_OUT(HasSubstr("\x1B[35m2:\x1B[0m\x1B[32mrandom\x1B[0m output\n"));
}

TEST_F(GsTest, StartExecuteGtestTaskSucceedAttemptToSearchOutputOfTestThatDoesNotExistAndViewListOfAllTests) {
  mock.MockProc(execs.kTests, exec_tests_successful);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("gs");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("gs0");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
  RunCmd("gs99");
  EXPECT_OUT(HasSubstrsInOrder(test_names));
}

TEST_F(GsTest, StartExecuteGtestTaskSucceedAndSearchOutputOfOneOfTheTests) {
  mock.MockProc(execs.kTests, exec_tests_successful);
  ASSERT_INIT_CDT();
  CMD("t1");
  CMD("gs1\n(some|data)");
  EXPECT_OUT(HasSubstr("\x1B[35m1:\x1B[0m\x1B[32msome\x1B[0m random\n"
                       "\x1B[35m2:\x1B[0m\x1B[32mdata\x1B[0m\n"));
  CMD("gs2\n(some|data)");
  EXPECT_OUT(HasSubstr("No matches found"));
}
