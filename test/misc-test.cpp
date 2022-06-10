#include "cdt.h"
#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>

using namespace testing;

class MiscTest: public CdtTest {};

TEST_F(MiscTest, StartAndDisplayHelp) {
  std::vector<std::string> expected_cmds = {"t<ind>", "tr<ind>", "d<ind>",
                                            "exec<ind>", "o<ind>", "s\t",
                                            "g<ind>", "gs<ind>", "gt<ind>",
                                            "gtr<ind>", "gd<ind>", "gf<ind>",
                                            "h\t"};
  ASSERT_CDT_STARTED();
  // Display help on unknown command
  EXPECT_CMDOUT("zz", HasSubstrs(expected_cmds));
  // Display help on explicit command
  EXPECT_CMDOUT("h", HasSubstrs(expected_cmds));
}

TEST_F(MiscTest, StartExecuteTaskAndAbortIt) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().is_long = true;
  ASSERT_CDT_STARTED();
  EXPECT_INTERRUPTED_CMDOUT("t1", HasSubstr("failed: return code: -1"));
  EXPECT_TRUE(WillWaitForInput(cdt));
  EXPECT_PROCS_EXACT(tasks.helloWorld.command);
}

TEST_F(MiscTest, StartExecuteSingleTaskAndRepeateTheLastCommandOnEnter) {
  ASSERT_CDT_STARTED();
  RunCmd("t1");
  RunCmd("");
  EXPECT_PROCS_EXACT(tasks.helloWorld.command, tasks.helloWorld.command);
}

TEST_F(MiscTest, StartAndExecuteRestartTask) {
  ASSERT_CDT_STARTED();
  InSequence seq;
  EXPECT_CALL(mock, SetEnv(kEnvVarLastCommand, "t7"));
  std::string tasks_config_path_str = paths.kTasksConfig.string();
  std::vector<const char*> expected_argv = {
    execs.kCdt.c_str(), tasks_config_path_str.c_str(), nullptr
  };
  EXPECT_CALL(mock, Exec(StrVecEq(expected_argv))).WillRepeatedly(Return(0));
  EXPECT_CMDOUT("t7", HasSubstr("Restarting program"));
}

TEST_F(MiscTest, StartAndFailToExecuteRestartTask) {
  std::string tasks_config_path_str = paths.kTasksConfig.string();
  std::vector<const char*> expected_argv = {
    execs.kCdt.c_str(), tasks_config_path_str.c_str(), nullptr
  };
  std::vector<std::string> expected_out = {
    "Restarting program",
    "Failed to restart: Exec format error"
  };
  EXPECT_CALL(mock, Exec(StrVecEq(expected_argv)))
      .WillRepeatedly(Return(ENOEXEC));
  ASSERT_CDT_STARTED();
  EXPECT_CMDOUT("t7", HasSubstrsInOrder(expected_out));
}

TEST_F(MiscTest, StartAndExecuteRestartTaskWithProfileSelected) {
  ASSERT_CDT_STARTED_WITH_PROFILE(profile1);
  InSequence seq;
  EXPECT_CALL(mock, SetEnv(kEnvVarLastCommand, "t7"));
  std::string tasks_config_path_str = paths.kTasksConfig.string();
  std::vector<const char*> expected_argv = {
    execs.kCdt.c_str(), tasks_config_path_str.c_str(), profile1.c_str(), nullptr
  };
  EXPECT_CALL(mock, Exec(StrVecEq(expected_argv))).WillRepeatedly(Return(0));
  EXPECT_CMDOUT("t7", HasSubstr("Restarting program"));
}
