#include "cdt.h"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace testing;

class MiscTest: public CdtTest {
protected:
  std::string cmd_primary;
  std::string tasks_config_path_str;
  std::vector<const char*> restart_args;

  void SetUp() override {
    tasks = {
        CreateTask("primary task", "echo primary task"),
        CreateTask("restart task", "__restart")};
    cmd_primary = tasks[0]["command"];
    tasks_config_path_str = paths.kTasksConfig.string();
    restart_args = {execs.kCdt.c_str(), tasks_config_path_str.c_str(), nullptr};
    CdtTest::SetUp();
  }
};

TEST_F(MiscTest, StartAndDisplayHelp) {
  std::vector<std::string> expected_cmds = {"t<ind>", "tr<ind>", "d<ind>",
                                            "exec<ind>", "o<ind>", "s\t",
                                            "g<ind>", "gs<ind>", "gt<ind>",
                                            "gtr<ind>", "gd<ind>", "gf<ind>",
                                            "h\t"};
  ASSERT_INIT_CDT();
  // Display help on unknown command
  RunCmd("zz");
  EXPECT_OUT(HasSubstrs(expected_cmds));
  // Display help on explicit command
  RunCmd("h");
  EXPECT_OUT(HasSubstrs(expected_cmds));
}

TEST_F(MiscTest, StartExecuteTaskAndAbortIt) {
  ProcessExec exec;
  exec.is_long = true;
  exec.output_lines = {tasks[0]["name"]};
  mock.MockProc(cmd_primary, exec);
  ASSERT_INIT_CDT();
  InterruptCmd("t1");
  EXPECT_OUT(HasSubstr("failed: return code: -1"));
  EXPECT_TRUE(WillWaitForInput(cdt));
  EXPECT_PROCS_EXACT(cmd_primary);
}

TEST_F(MiscTest, StartAndExecuteRestartTask) {
  ASSERT_INIT_CDT();
  InSequence seq;
  EXPECT_CALL(mock, SetEnv(kEnvVarLastCommand, "t2"));
  EXPECT_CALL(mock, Exec(StrVecEq(restart_args))).WillRepeatedly(Return(0));
  RunCmd("t2");
  EXPECT_OUT(HasSubstr("Restarting program"));
}

TEST_F(MiscTest, StartAndFailToExecuteRestartTask) {
  std::vector<std::string> expected_out = {
    "Restarting program",
    "Failed to restart: Exec format error"
  };
  EXPECT_CALL(mock, Exec(StrVecEq(restart_args)))
      .WillRepeatedly(Return(ENOEXEC));
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUT(HasSubstrsInOrder(expected_out));
}

TEST_F(MiscTest, StartAndExecuteRestartTaskWithProfileSelected) {
  profiles = {{{"name", "profile 1"}}, {{"name", "profile 2"}}};
  std::string profile_name = profiles[1]["name"];
  args.push_back(profile_name);
  MockTasksConfig();
  ASSERT_INIT_CDT();
  InSequence seq;
  EXPECT_CALL(mock, SetEnv(kEnvVarLastCommand, "t2"));
  restart_args.back() = profile_name.c_str();
  restart_args.push_back(nullptr);
  EXPECT_CALL(mock, Exec(StrVecEq(restart_args))).WillRepeatedly(Return(0));
  RunCmd("t2");
  EXPECT_OUT(HasSubstr("Restarting program"));
}
