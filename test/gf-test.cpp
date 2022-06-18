#include "json.hpp"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>
#include <vector>

using namespace testing;

class GfTest: public CdtTest {
protected:
  std::vector<nlohmann::json> tasks;

  void SetUp() override {
    Init();
    tasks = {
        CreateTaskAndProcess("pre task"),
        CreateTask("run tests", "__gtest " + execs.kTests, {"pre task"})};
    mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
  }
};

TEST_F(GfTest, StartAttemptToExecuteGoogleTestsWithFilterTargetingTaskThatDoesNotExistAndViewListOfTask) {
  std::vector<std::string> list_of_tasks = {"1 \"pre task\"",
                                            "2 \"run tests\""};
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("gf");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks));
  RunCmd("gf0");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks));
  RunCmd("gf99");
  EXPECT_OUT(HasSubstrsInOrder(list_of_tasks));
}

TEST_F(GfTest, StartAndExecuteGtestTaskWithPreTasksWithGtestFilter) {
  std::string cmd = execs.kTests + WITH_GT_FILTER("suite1.*");
  std::vector<DummyTestSuite> suite = {
      DummyTestSuite{"suite1", {
          DummyTest{"test1"},
          DummyTest{"test2"}}}};
  ProcessExec exec;
  exec.output_lines = CreateTestOutput(suite);
  mock.MockProc(cmd, exec);
  ASSERT_STARTED(TestCdt(tasks, {}, args));
  RunCmd("gf2\nsuite1.*");
  EXPECT_OUT(HasSubstr(TASK_COMPLETE("suite1.*")));
  EXPECT_PROCS_EXACT(tasks[0]["command"], cmd);
}
