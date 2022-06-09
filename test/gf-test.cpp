#include "test-lib.h"

class GfTest: public CdtTest {};

TEST_F(GfTest, StartAttemptToExecuteGoogleTestsWithFilterTargetingTaskThatDoesNotExistAndViewListOfTask) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("gf");
  EXPECT_CMD("gf0");
  EXPECT_CMD("gf99");
}

TEST_F(GfTest, StartAndExecuteGtestTaskWithGtestFilter) {
  ProcessExec filtered_tests;
  filtered_tests.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "[==========] Running 2 tests from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 2 tests from test_suit_1",
    "[ RUN      ] test_suit_1.test1",
    OUT_LINKS_NOT_HIGHLIGHTED(),
    "[       OK ] test_suit_1.test1 (0 ms)",
    "[ RUN      ] test_suit_1.test2",
    "[       OK ] test_suit_1.test2 (0 ms)",
    "[----------] 2 tests from test_suit_1 (0 ms total)",
    "",
    "[----------] Global test environment tear-down",
    "[==========] 2 tests from 1 test suite ran. (0 ms total)",
    "[  PASSED  ] 2 tests.",
  };
  mock.cmd_to_process_execs[execs.kTests + WITH_GT_FILTER("test_suit_1.*")]
      .push_back(filtered_tests);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("gf8\ntest_suit_1.*");
}
