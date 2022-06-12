#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class DTest: public CdtTest {};

TEST_F(DTest, StartAttemptToDebugTaskWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
  mock.MockReadFile(paths.kUserConfig);
  ASSERT_CDT_STARTED();
  CMD("d");
  EXPECT_OUT(HasSubstr("debug_command"));
  EXPECT_OUT(HasPath(paths.kUserConfig));
}

TEST_F(DTest, StartAttemptDebugTaskThatDoesNotExistAndViewListOfAllTasks) {
  ASSERT_CDT_STARTED();
  CMD("d");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks_in_ui));
  CMD("d0");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks_in_ui));
  CMD("d99");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks_in_ui));
}

TEST_F(DTest, StartDebugPrimaryTaskWithPreTasks) {
  std::string debugger_call = WITH_DEBUG("echo primary task");
  mock.cmd_to_process_execs[debugger_call].push_back(ProcessExec{});
  ASSERT_CDT_STARTED();
  CMD("d2");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("primary task")));
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     "echo pre task 1", "echo pre task 2", debugger_call);
}

TEST_F(DTest, StartAndFailToDebugTask) {
  std::string debugger_call = WITH_DEBUG("echo primary task");
  mock.cmd_to_process_execs[debugger_call].push_back(failed_debug_exec);
  ASSERT_CDT_STARTED();
  CMD("d2");
  EXPECT_OUT(HasSubstr(failed_debug_exec.output_lines.front()));
  EXPECT_OUT(HasSubstr(TASK_FAILED("primary task",
                                   failed_debug_exec.exit_code)));
}

TEST_F(DTest, StartDebugGtestPrimaryTaskWithPreTasks) {
  std::string debugger_call = WITH_DEBUG("tests");
  mock.cmd_to_process_execs[debugger_call].push_back(ProcessExec{});
  ASSERT_CDT_STARTED();
  CMD("d10");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run tests with pre tasks")));
  EXPECT_PROCS_EXACT("echo pre pre task 1", "echo pre pre task 2",
                     debugger_call);
}

TEST_F(DTest, StartAndFailToDebugGtestTask) {
  std::string debugger_call = WITH_DEBUG("tests");
  mock.cmd_to_process_execs[debugger_call].push_back(failed_debug_exec);
  ASSERT_CDT_STARTED();
  CMD("d10");
  EXPECT_OUT(HasSubstr(failed_debug_exec.output_lines.front()));
}
