#include "test-lib.h"
#include <gmock/gmock-matchers.h>
#include <string>

using namespace testing;

class OTest: public CdtTest {
protected:
  ProcessExec exec_with_links;
  std::string cmd_primary, cmd_pre_task;

  void SetUp() override {
    tasks = {
        CreateTask("pre task", "echo pre task"),
        CreateTask("primary task", "echo primary task", {"pre task"})};
    cmd_pre_task = tasks[0]["command"];
    cmd_primary = tasks[1]["command"];
    exec_with_links.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    mock.MockProc(cmd_pre_task, exec_with_links);
    mock.MockProc(cmd_primary, exec_with_links);
    Init();
  }
};

TEST_F(OTest, StartExecuteTaskAndFailToOpenLinksFromOutput) {
  ProcessExec exec;
  exec.exit_code = 1;
  exec.output_lines = {"failed to open file"};
  mock.MockProc(execs.kEditor + " /a/b/c:10", exec);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  RunCmd("o1");
  EXPECT_OUT(HasSubstr(exec.output_lines.front()));
}

TEST_F(OTest, StartExecuteTaskAndOpenLinksFromOutput) {
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartFailToExecuteTaskWithLinksAndOpenLinksFromOutput) {
  exec_with_links.exit_code = 1;
  mock.MockProc(cmd_primary, exec_with_links);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartFailToExecutePreTaskOfTaskAndOpenLinksFromOutput) {
  exec_with_links.exit_code = 1;
  mock.MockProc(cmd_pre_task, exec_with_links);
  ASSERT_INIT_CDT();
  RunCmd("t2");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(OTest, StartExecuteTaskWithLinksInOutputAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartFailToExecuteTaskWithLinksAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  exec_with_links.exit_code = 1;
  mock.MockProc(cmd_pre_task, exec_with_links);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartFailToExecutePreTaskOfTaskAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  exec_with_links.exit_code = 1;
  mock.MockProc(cmd_pre_task, exec_with_links);
  ASSERT_INIT_CDT();
  CMD("t2");
  EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(OTest, StartAttemptToOpenNonExistentLinkAndViewTaskOutput) {
  std::string no_file_links = "No file links in the output";
  ASSERT_INIT_CDT();
  RunCmd("o1");
  EXPECT_OUT(HasSubstr(no_file_links));
  RunCmd("o");
  EXPECT_OUT(HasSubstr(no_file_links));
}

TEST_F(OTest, StartAttemptToOpenLinkWhileOpenInEditorCommandIsNotSpecifiedAndSeeError) {
  mock.MockReadFile(paths.kUserConfig);
  ASSERT_INIT_CDT();
  RunCmd("o");
  EXPECT_OUT(HasSubstr("'open_in_editor_command' is not specified"));
  EXPECT_OUT(HasPath(paths.kUserConfig));
}

#ifdef _WIN32
TEST_F(OTest, StartExecuteTaskWithWindowsLinksAndOpenLinksFromOutput) {
  std::string open_link1 =
      execs.kEditor + " C:\\Program Files\\My App\\config.txt:35:55";
  std::string open_link2 =
      execs.kEditor + " D:\\Games\\config.ini:32:15";
  mock.MockProc(open_link1);
  mock.MockProc(open_link2);
  exec_with_links.output_lines = {
      "my file: C:\\Program Files\\My App\\config.txt(35,55)",
      "line without links", "D:\\Games\\config.ini:32:15 contains errors"};
  mock.MockProc(cmd_pre_task, exec_with_links);
  ASSERT_INIT_CDT();
  RunCmd("t1");
  RunCmd("o1");
  RunCmd("o2");
  EXPECT_PROCS_EXACT(cmd_pre_task, open_link1, open_link2);
}
#endif
