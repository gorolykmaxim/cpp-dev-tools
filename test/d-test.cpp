#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace testing;

class DTest: public CdtTest {
protected:
  std::string cmd_with_debugger = WITH_DEBUG("echo primary task");
  std::string tests_with_debugger = WITH_DEBUG(execs.kTests);
  ProcessExec failed_debug_exec;
  std::vector<nlohmann::json> tasks;

  void SetUp() override {
    failed_debug_exec.exit_code = 1;
    failed_debug_exec.output_lines = {"failed to launch debugger"};
    failed_debug_exec.stderr_lines.insert(0);
    tasks = {
      CreateTaskAndProcess("pre task"),
      CreateTaskAndProcess("primary task", {"pre task"}),
      CreateTask("run tests", "__gtest " + execs.kTests, {"pre task"})
    };
    Init();
  }
};

TEST_F(DTest, StartAttemptToDebugTaskWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
  mock.MockReadFile(paths.kUserConfig);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("d");
  EXPECT_OUT(HasSubstr("debug_command"));
  EXPECT_OUT(HasPath(paths.kUserConfig));
}

TEST_F(DTest, StartAttemptDebugTaskThatDoesNotExistAndViewListOfAllTasks) {
  std::vector<std::string> task_names;
  for (nlohmann::json& t: tasks) {
    task_names.push_back(t["name"]);
  }
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("d");
  EXPECT_OUT(HasSubstrsInOrder(task_names));
  RunCmd("d0");
  EXPECT_OUT(HasSubstrsInOrder(task_names));
  RunCmd("d99");
  EXPECT_OUT(HasSubstrsInOrder(task_names));
}

TEST_F(DTest, StartDebugPrimaryTaskWithPreTasks) {
  mock.MockProc(cmd_with_debugger);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("d2");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("primary task")));
  EXPECT_PROCS_EXACT("echo pre task", cmd_with_debugger);
}

TEST_F(DTest, StartAndFailToDebugTask) {
  mock.MockProc(cmd_with_debugger, failed_debug_exec);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("d2");
  EXPECT_OUT(HasSubstr(failed_debug_exec.output_lines.front()));
  EXPECT_OUT(HasSubstr(TASK_FAILED("primary task",
                                   failed_debug_exec.exit_code)));
}

TEST_F(DTest, StartDebugGtestPrimaryTaskWithPreTasks) {
  mock.MockProc(tests_with_debugger);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("d3");
  EXPECT_OUT(HasSubstr("Debugger started"));
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("run tests")));
  EXPECT_PROCS_EXACT("echo pre task", tests_with_debugger);
}

TEST_F(DTest, StartAndFailToDebugGtestTask) {
  mock.MockProc(tests_with_debugger, failed_debug_exec);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("d3");
  EXPECT_OUT(HasSubstr(failed_debug_exec.output_lines.front()));
}
