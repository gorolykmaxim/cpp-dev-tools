#include "test-lib.h"
#include <gtest/gtest.h>

class MiscTest: public CdtTest {};

TEST_F(MiscTest, StartAndDisplayHelp) {
  std::string expected_help =
      "\x1B[32mUser commands:\x1B[0m\n"
      "t<ind>\t\tExecute the task with the specified index\n"
      "tr<ind>\t\tKeep executing the task with the specified index until it fails\n"
      "d<ind>\t\tExecute the task with the specified index with a debugger attached\n"
      "exec<ind>\tChange currently selected execution (gets reset to the most recent one after every new execution)\n"
      "o<ind>\t\tOpen the file link with the specified index in your code editor\n"
      "s\t\tSearch through output of the selected executed task with the specified regular expression\n"
      "g<ind>\t\tDisplay output of the specified google test\n"
      "gs<ind>\t\tSearch through output of the specified google test with the specified regular expression\n"
      "gt<ind>\t\tRe-run the google test with the specified index\n"
      "gtr<ind>\tKeep re-running the google test with the specified index until it fails\n"
      "gd<ind>\t\tRe-run the google test with the specified index with debugger attached\n"
      "gf<ind>\t\tRun google tests of the task with the specified index with a specified --gtest_filter\n"
      "h\t\tDisplay list of user commands\n";
  EXPECT_CDT_STARTED();
  // Display help on unknown command
  EXPECT_CMD("zz");
  // Display help on explicit command
  EXPECT_CMD("h");
}

TEST_F(MiscTest, StartExecuteTaskAndAbortIt) {
  mock.cmd_to_process_execs[execs.kHelloWorld][0].is_long = true;
  EXPECT_CDT_STARTED();
  EXPECT_INTERRUPTED_CMD("t1");
}

TEST_F(MiscTest, StartAndExit) {
  EXPECT_CDT_STARTED();
  EXPECT_FALSE(ctrl_c_handler());
}

TEST_F(MiscTest, StartExecuteSingleTaskAndRepeateTheLastCommandOnEnter) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("");
}

TEST_F(MiscTest, StartAndExecuteRestartTask) {
  testing::InSequence seq;
  EXPECT_CALL(mock, SetEnv("LAST_COMMAND", "t7"));
  std::vector<const char*> expected_argv = {
    execs.kCdt.c_str(), paths.kTasksConfig.c_str(), nullptr
  };
  EXPECT_CALL(mock, Exec(StrVecEq(expected_argv)))
      .WillRepeatedly(testing::Return(0));
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t7");
}

TEST_F(MiscTest, StartAndFailToExecuteRestartTask) {
  std::vector<const char*> expected_argv = {
    execs.kCdt.c_str(), paths.kTasksConfig.c_str(), nullptr
  };
  EXPECT_CALL(mock, Exec(StrVecEq(expected_argv)))
      .WillRepeatedly(testing::Return(ENOEXEC));
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t7");
}

TEST_F(MiscTest, StartAndExecuteRestartTaskWithProfileSelected) {
  EXPECT_CDT_STARTED_WITH_PROFILE(profile1);
  testing::InSequence seq;
  EXPECT_CALL(mock, SetEnv("LAST_COMMAND", "t7"));
  std::vector<const char*> expected_argv = {
    execs.kCdt.c_str(), paths.kTasksConfig.c_str(), profile1.c_str(), nullptr
  };
  EXPECT_CALL(mock, Exec(StrVecEq(expected_argv)))
      .WillRepeatedly(testing::Return(0));
  EXPECT_CMD("t7");
}
