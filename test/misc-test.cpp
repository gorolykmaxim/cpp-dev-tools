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
  CMD("zz");
  EXPECT_OUT(HasSubstrs(expected_cmds));
  // Display help on explicit command
  CMD("h");
  EXPECT_OUT(HasSubstrs(expected_cmds));
}

TEST_F(MiscTest, StartExecuteTaskAndAbortIt) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().is_long = true;
  ASSERT_CDT_STARTED();
  INTERRUPT_CMD("t1");
  EXPECT_OUT(HasSubstr("failed: return code: -1"));
  EXPECT_TRUE(WillWaitForInput(cdt));
  EXPECT_PROCS_EXACT(execs.kHelloWorld);
}

TEST_F(MiscTest, StartExecuteSingleTaskAndRepeateTheLastCommandOnEnter) {
  ASSERT_CDT_STARTED();
  RunCmd("t1");
  RunCmd("");
  EXPECT_PROCS_EXACT(execs.kHelloWorld, execs.kHelloWorld);
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
  CMD("t7");
  EXPECT_OUT(HasSubstr("Restarting program"));
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
  CMD("t7");
  EXPECT_OUT(HasSubstrsInOrder(expected_out));
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
  CMD("t7");
  EXPECT_OUT(HasSubstr("Restarting program"));
}
