#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>

using namespace testing;

class GdTest: public CdtTest {};

TEST_F(GdTest, StartAttemptToRerunGtestWithDebuggerWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
  mock.MockReadFile(paths.kUserConfig);
  ASSERT_CDT_STARTED();
  CMD("gd");
  EXPECT_OUT(HasSubstr("'debug_command' is not specified"));
  EXPECT_OUT(HasPath(paths.kUserConfig));
}

TEST_F(GdTest, StartAttemptToRerunGtestWithDebuggerWhenNoTestsHaveBeenExecutedYet) {
  ASSERT_CDT_STARTED();
  CMD("gd");
  EXPECT_OUT(HasSubstr("No google tests have been executed yet."));
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksFailAttemptToRerunTestThatDoesNotExistWithDebuggerAndViewListOfFailedTests) {
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gd");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gd0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  CMD("gd99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_failed));
  // Check that we don't attempt to execute any debugger calls
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     execs.kTests);
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksFailAndRerunFailedTestWithDebugger) {
  std::string pre_task1 = "echo pre pre task 1";
  std::string pre_task2 = "echo pre pre task 2";
  std::string rerun1 =
      WITH_DEBUG("tests" WITH_GT_FILTER("failed_test_suit_1.test1"));
  std::string rerun2 =
      WITH_DEBUG("tests" WITH_GT_FILTER("failed_test_suit_2.test1"));
  mock.cmd_to_process_execs[rerun1].push_back(ProcessExec{});
  mock.cmd_to_process_execs[rerun2].push_back(ProcessExec{});
  mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gd1");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("failed_test_suit_1.test1")));
  CMD("gd2");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("failed_test_suit_2.test1")));
  EXPECT_PROCS_EXACT(pre_task1, pre_task2, execs.kTests, pre_task1, pre_task2,
                     rerun1, pre_task1, pre_task2, rerun2);
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRerunTestThatDoesNotExistWithDebuggerAndViewListOfAllTests) {
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gd");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gd0");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  CMD("gd99");
  EXPECT_OUT(HasSubstrsInOrder(test_list_successful));
  // Check that we don't attempt to execute any debugger calls
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     execs.kTests);
}

TEST_F(GdTest, StartExecuteGtestTaskWithPreTasksSucceedAndRerunOneOfTestsWithDebugger) {
  std::string pre_task1 = "echo pre pre task 1";
  std::string pre_task2 = "echo pre pre task 2";
  std::string rerun1 =
      WITH_DEBUG("tests" WITH_GT_FILTER("test_suit_1.test1"));
  std::string rerun2 =
      WITH_DEBUG("tests" WITH_GT_FILTER("test_suit_1.test2"));
  mock.cmd_to_process_execs[rerun1].push_back(ProcessExec{});
  mock.cmd_to_process_execs[rerun2].push_back(ProcessExec{});
  ASSERT_CDT_STARTED();
  CMD("t10");
  CMD("gd1");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("test_suit_1.test1")));
  CMD("gd2");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("test_suit_1.test2")));
  EXPECT_PROCS_EXACT(pre_task1, pre_task2, execs.kTests, pre_task1, pre_task2,
                     rerun1, pre_task1, pre_task2, rerun2);
}
