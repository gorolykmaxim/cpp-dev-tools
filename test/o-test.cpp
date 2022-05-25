#include "test-lib.h"

class OTest: public CdtTest {};

TEST_F(OTest, StartExecuteTaskAndFailToOpenLinksFromOutput) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  ProcessExec exec;
  exec.exit_code = 1;
  exec.output_lines = {"failed to open file\n"};
  mock.cmd_to_process_execs[execs.kEditor + " /a/b/c:10"].push_back(exec);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_CMD("o1");
}

TEST_F(OTest, StartExecuteTaskAndOpenLinksFromOutput) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartFailToExecuteTaskWithLinksAndOpenLinksFromOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.exit_code = 1;
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartFailToExecutePreTaskOfTaskAndOpenLinksFromOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.exit_code = 1;
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t3");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartExecuteTaskWithLinksInOutputAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartFailToExecuteTaskWithLinksAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.exit_code = 1;
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t1");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartFailToExecutePreTaskOfTaskAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.exit_code = 1;
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t3");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  std::string expected_output = "\x1B[32mNo file links in the output\x1B[0m\n";
  EXPECT_CDT_STARTED();
  EXPECT_CMD("o1");
  EXPECT_CMD("o");
}

TEST_F(OTest, StartAttemptToOpenLinkWhileOpenInEditorCommandIsNotSpecifiedAndSeeError) {
  mock.MockReadFile(paths.kUserConfig);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("o");
}
