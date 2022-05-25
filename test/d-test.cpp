#include "test-lib.h"

class DTest: public CdtTest {};

TEST_F(DTest, StartAttemptToDebugTaskWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
  mock.MockReadFile(paths.kUserConfig);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("d");
}

TEST_F(DTest, StartAttemptDebugTaskThatDoesNotExistAndViewListOfAllTasks) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("d");
  EXPECT_CMD("d0");
  EXPECT_CMD("d99");
}

TEST_F(DTest, StartDebugPrimaryTaskWithPreTasks) {
  EXPECT_PROC(WITH_DEBUG("echo primary task"));
  EXPECT_CDT_STARTED();
  EXPECT_CMD("d2");
}

TEST_F(DTest, StartAndFailToDebugTask) {
  std::string debugger_call = WITH_DEBUG("echo primary task");
  mock.cmd_to_process_execs[debugger_call].push_back(failed_debug_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("d2");
}

TEST_F(DTest, StartDebugGtestPrimaryTaskWithPreTasks) {
  EXPECT_PROC(WITH_DEBUG("tests"));
  EXPECT_CDT_STARTED();
  EXPECT_CMD("d10");
}

TEST_F(DTest, StartAndFailToDebugGtestTask) {
  std::string debugger_call = WITH_DEBUG("tests");
  mock.cmd_to_process_execs[debugger_call].push_back(failed_debug_exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("d10");
}
