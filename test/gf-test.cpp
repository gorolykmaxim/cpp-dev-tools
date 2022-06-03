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
    "Running main() from /lib/gtest_main.cc\n",
    "[==========] Running 2 tests from 1 test suite.\n",
    "[----------] Global test environment set-up.\n",
    "[----------] 2 tests from test_suit_1\n",
    "[ RUN      ] test_suit_1.test1\n",
    out_links,
    "[       OK ] test_suit_1.test1 (0 ms)\n",
    "[ RUN      ] test_suit_1.test2\n",
    "[       OK ] test_suit_1.test2 (0 ms)\n",
    "[----------] 2 tests from test_suit_1 (0 ms total)\n\n",
    "[----------] Global test environment tear-down\n",
    "[==========] 2 tests from 1 test suite ran. (0 ms total)\n",
    "[  PASSED  ] 2 tests.\n",
  };
  mock.cmd_to_process_execs[execs.kTests + WITH_GT_FILTER("test_suit_1.*")]
      .push_back(filtered_tests);
  EXPECT_CDT_STARTED();
  EXPECT_CMD("gf8\ntest_suit_1.*");
}
