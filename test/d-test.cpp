#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class DTest: public CdtTest {};

TEST_F(DTest, StartAttemptToDebugTaskWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
  mock.MockReadFile(paths.kUserConfig);
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("d", HasSubstr("debug_command"), HasPath(paths.kUserConfig));
}

TEST_F(DTest, StartAttemptDebugTaskThatDoesNotExistAndViewListOfAllTasks) {
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("d", HasSubstrsInOrder(list_of_tasks_in_ui));
  EXPECT_CMDOUT("d0", HasSubstrsInOrder(list_of_tasks_in_ui));
  EXPECT_CMDOUT("d99", HasSubstrsInOrder(list_of_tasks_in_ui));
}

TEST_F(DTest, StartDebugPrimaryTaskWithPreTasks) {
  std::string debugger_call = WITH_DEBUG(tasks.primaryTask.command);
  mock.cmd_to_process_execs[debugger_call].push_back(ProcessExec{});
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("d2", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK(tasks.prePreTask1), RUNNING_PRE_TASK(tasks.prePreTask2),
      RUNNING_PRE_TASK(tasks.preTask1), RUNNING_PRE_TASK(tasks.preTask2),
      RUNNING_TASK(tasks.primaryTask), "Debugger started",
      TASK_COMPLETE(tasks.primaryTask)}));
  EXPECT_PROCS_EXACT(tasks.prePreTask1.command, tasks.prePreTask2.command,
                     tasks.preTask1.command, tasks.preTask2.command,
                     debugger_call);
}

TEST_F(DTest, StartAndFailToDebugTask) {
  std::string debugger_call = WITH_DEBUG(tasks.primaryTask.command);
  mock.cmd_to_process_execs[debugger_call].push_back(failed_debug_exec);
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("d2", HasSubstr(failed_debug_exec.output_lines.front()),
                HasSubstr(TASK_FAILED(tasks.primaryTask,
                                      failed_debug_exec.exit_code)));
}

TEST_F(DTest, StartDebugGtestPrimaryTaskWithPreTasks) {
  std::string debugger_call = WITH_DEBUG("tests");
  mock.cmd_to_process_execs[debugger_call].push_back(ProcessExec{});
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("d10", HasSubstrsInOrder(std::vector<std::string>{
      RUNNING_PRE_TASK(tasks.prePreTask1), RUNNING_PRE_TASK(tasks.prePreTask2),
      RUNNING_TASK(tasks.runTestsWithPreTasks), "Debugger started",
      TASK_COMPLETE(tasks.runTestsWithPreTasks)}));
  EXPECT_PROCS_EXACT(tasks.prePreTask1.command, tasks.prePreTask2.command,
                     debugger_call);
}

TEST_F(DTest, StartAndFailToDebugGtestTask) {
  std::string debugger_call = WITH_DEBUG(execs.kTests);
  mock.cmd_to_process_execs[debugger_call].push_back(failed_debug_exec);
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("d10", HasSubstr(failed_debug_exec.output_lines.front()));
}
