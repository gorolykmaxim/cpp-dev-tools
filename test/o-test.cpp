#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>

using namespace testing;

class OTest: public CdtTest {};

TEST_F(OTest, StartExecuteTaskAndFailToOpenLinksFromOutput) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  ProcessExec exec;
  exec.exit_code = 1;
  exec.output_lines = {"failed to open file"};
  mock.cmd_to_process_execs[execs.kEditor + " /a/b/c:10"].push_back(exec);
  ASSERT_CDT_STARTED();
  CMD("t1");
  CMD("o1");
  EXPECT_OUT(HasSubstr(exec.output_lines.front()));
}

TEST_F(OTest, StartExecuteTaskAndOpenLinksFromOutput) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartFailToExecuteTaskWithLinksAndOpenLinksFromOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.exit_code = 1;
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartFailToExecutePreTaskOfTaskAndOpenLinksFromOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.exit_code = 1;
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t3");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartExecuteTaskWithLinksInOutputAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {
    OUT_LINKS_NOT_HIGHLIGHTED()
  };
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartFailToExecuteTaskWithLinksAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
  exec.exit_code = 1;
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t1");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartFailToExecutePreTaskOfTaskAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.exit_code = 1;
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  ASSERT_CDT_STARTED();
  CMD("t3");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  std::string no_file_links = "No file links in the output";
  ASSERT_CDT_STARTED();
  CMD("o1");
  EXPECT_OUT(HasSubstr(no_file_links));
  CMD("o");
  EXPECT_OUT(HasSubstr(no_file_links));
}

TEST_F(OTest, StartAttemptToOpenLinkWhileOpenInEditorCommandIsNotSpecifiedAndSeeError) {
  mock.MockReadFile(paths.kUserConfig);
  ASSERT_CDT_STARTED();
  CMD("o");
  EXPECT_OUT(HasSubstr("'open_in_editor_command' is not specified"));
  EXPECT_OUT(HasPath(paths.kUserConfig));
}
