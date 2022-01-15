#include "blockingconcurrentqueue.h"
#include "concurrentqueue.h"
#include "process.hpp"
#include <cstddef>
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <ios>
#include <iostream>
#include <filesystem>
#include <iterator>
#include <memory>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unordered_map>
#include <vector>

static std::function<void(const char*, size_t)> write_to(moodycamel::BlockingConcurrentQueue<std::string>& output, std::string& line_buffer) {
    return [&output, &line_buffer] (const char* data, size_t size) {
        const auto to_process = line_buffer + std::string(data, size);
        std::stringstream line;
        for (const auto c: to_process) {
            if (c == '\n') {
                output.enqueue(line.str());
                line = std::stringstream();
            } else {
                line << c;
            }
        }
        line_buffer = line.str();
    };
}

struct cdt_test: public testing::Test {
protected:
    const std::string TIMEOUT_ERROR_MSG = "TIMED-OUT WAITING FOR OUTPUT FROM CDT";
    const std::string TWO_PRE_TASKS_OUTPUT = "pre pre task 1\npre pre task 2\n";
    const std::string CDT_PREFIX = "\x1B[32m(cdt) \x1B[0m";
    const std::string GTEST_FILTER_PREFIX = "\x1B[32m--gtest_filter=\x1B[0m";
    const std::string REGEX_PREFIX = "\x1B[32mRegular expression: \x1B[0m";
    const std::string GTEST_PROGRESS_FINISH = "\33[2K\r";
    const std::regex GTEST_EXEC_TIME_REGEX = std::regex("[0-9]+ ms");
    const std::filesystem::path cwd = std::filesystem::path(BINARY_DIR);
    const std::filesystem::path test_configs_dir = std::filesystem::path(TEST_CONFIGS_DIR);
    const std::filesystem::path test_homes_dir = std::filesystem::path(TEST_HOMES_DIR);
    const std::filesystem::path cmd_out_file = cwd / "cmd-test-output.txt";
    const std::string DEFAULT_USER_CONFIG_CONTENT =
    "{\n"
    "  // Open file links from the output in Sublime Text:\n"
    "  //\"open_in_editor_command\": \"subl {}\"\n"
    "  // Open file links from the output in VSCode:\n"
    "  //\"open_in_editor_command\": \"code {}\"\n"
    "}\n";

    moodycamel::BlockingConcurrentQueue<std::string> output;
    std::string stdout_buffer, stderr_buffer;
    std::unique_ptr<TinyProcessLib::Process> proc;

    virtual void SetUp() override {
        remove_tmp_files();
    }
    virtual void TearDown() override {
        if (proc) {
            int d;
            while (!proc->try_get_exit_status(d)) {
                proc->kill();
            }
        }
        remove_tmp_files();
    }
    void remove_tmp_files() {
        for (const auto& file: std::filesystem::directory_iterator(test_configs_dir)) {
            if (file.path().extension() == ".tmp") {
                std::filesystem::remove(file.path());
            }
        }
        std::filesystem::remove(cmd_out_file);
        std::filesystem::remove(to_absolute_user_config_path("no-config"));
    }
    void run_cdt(const std::string& config_name, const std::string& home_dir) {
        const std::vector<std::string> args = {cwd / BINARY_NAME, test_configs_dir / config_name};
        const std::unordered_map<std::string, std::string> env = {
            {"HOME", test_homes_dir / home_dir},
            {"OUTPUT_FILE", cmd_out_file},
            {"GTEST_BINARY_NAME", cwd / GTEST_BINARY_NAME}
        };
        proc = std::make_unique<TinyProcessLib::Process>(args, "", env, write_to(output, stdout_buffer), write_to(output, stderr_buffer), true);
    }
    void run_cmd(const std::string& cmd) {
        proc->write(cmd + "\n");
    }
    std::string to_absolute_user_config_path(const std::string& user_config_dir) {
        return (test_homes_dir / user_config_dir / ".cpp-dev-tools.json").string();
    }
    std::string to_absolute_tasks_config_path(const std::string& tasks_config) {
        return (test_configs_dir / tasks_config).string();
    }
    std::string is_invalid_error_message(const std::string& config_path) {
        using namespace std::string_literals;
        return "\x1B[31m"s + config_path + " is invalid:";
    }
    void print_lines() {
        while (true) {
            const auto line = get_out_line();
            if (line == TIMEOUT_ERROR_MSG) {
                break;
            }
            EXPECT_EQ("", line);
        }
    }
    std::string get_out_line() {
        std::string msg;
        if (!output.wait_dequeue_timed(msg, 1000000)) {
            msg = TIMEOUT_ERROR_MSG;
        }
        // Ignore prefix's in output when waiting for a command
        remove_before_including(msg, CDT_PREFIX);
        remove_before_including(msg, GTEST_FILTER_PREFIX);
        remove_before_including(msg, REGEX_PREFIX);
        // Ignore gtest progress line (that keeps updating itself). We will only see the end result, printed over that line.
        remove_before_including(msg, GTEST_PROGRESS_FINISH);
        msg = std::regex_replace(msg, GTEST_EXEC_TIME_REGEX, "X ms");
        std::cout << msg << std::endl;
        return msg;
    }
    void remove_before_including(std::string& str, const std::string& before) {
        const auto pos = str.rfind(before);
        if (pos != std::string::npos) {
            str = str.substr(pos + before.size());
        }
    }
    void interrupt_current_task() {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        proc->signal(SIGINT);
    }
    std::string read_file(const std::filesystem::path& path) {
        std::ifstream file(path);
        if (!file) return "";
        file >> std::noskipws;
        return std::string(std::istream_iterator<char>(file), std::istream_iterator<char>());
    }
};

#define ASSERT_LINE(LINE) ASSERT_EQ(LINE, get_out_line())
#define ASSERT_CMD_OUT(OUT) ASSERT_EQ(OUT, read_file(cmd_out_file))
#define ASSERT_HELP_PROMPT_DISPLAYED() ASSERT_LINE("Type \x1B[32mh\x1B[0m to see list of all the user commands.")
#define ASSERT_TASK_LIST_DISPLAYED()\
    ASSERT_LINE("\x1B[32mTasks:\x1B[0m");\
    ASSERT_LINE("1 \"hello world\"");\
    ASSERT_LINE("2 \"primary task\"");\
    ASSERT_LINE("3 \"pre task 1\"");\
    ASSERT_LINE("4 \"pre task 2\"");\
    ASSERT_LINE("5 \"pre pre task 1\"");\
    ASSERT_LINE("6 \"pre pre task 2\"");\
    ASSERT_LINE("7 \"fail pre task\"");\
    ASSERT_LINE("8 \"primary task with fail pre task\"");\
    ASSERT_LINE("9 \"long task\"");\
    ASSERT_LINE("10 \"current working directory\"");\
    ASSERT_LINE("11 \"restart\"");\
    ASSERT_LINE("12 \"primary task with links\"");\
    ASSERT_LINE("13 \"fail primary task with links\"");\
    ASSERT_LINE("14 \"primary task with fail pre task with links\"");\
    ASSERT_LINE("15 \"gtest with no tests\"");\
    ASSERT_LINE("16 \"gtest with non-existent binary\"");\
    ASSERT_LINE("17 \"gtest with non-gtest binary\"");\
    ASSERT_LINE("18 \"gtest with long non-gtest binary\"");\
    ASSERT_LINE("19 \"gtest exit in the middle with 0 exit code\"");\
    ASSERT_LINE("20 \"gtest exit in the middle with 1 exit code\"");\
    ASSERT_LINE("21 \"task with gtest pre task that exits in the middle with 0 exit code\"");\
    ASSERT_LINE("22 \"gtest with multiple test suites\"");\
    ASSERT_LINE("23 \"gtest with multiple failed test suites\"");\
    ASSERT_LINE("24 \"task woth gtest pre task with multiple test suites\"");\
    ASSERT_LINE("25 \"task woth gtest pre task with multiple failed test suites\"");\
    ASSERT_LINE("26 \"gtest with long test\"");\
    ASSERT_LINE("27 \"gtest with long test and failures\"");\
    ASSERT_LINE("28 \"gtest with sporadically failing test\"");\
    ASSERT_LINE("29 \"gtest with single failed test\"");\
    ASSERT_LINE("30 \"primary task with failed gtest pre task with single failed test\"");\
    ASSERT_LINE("31 \"gtest with multiple test suites and pre tasks\"");\
    ASSERT_LINE("32 \"gtest with multiple failed test suites and pre tasks\"");\
    ASSERT_LINE("33 \"gtest with sporadically failing test with pre tasks\"");\
    ASSERT_LINE("34 \"gtest task with sporadically failing pre task\"");\
    ASSERT_LINE("35 \"gtest with long test with pre tasks\"")
#define ASSERT_HELP_DISPLAYED()\
    ASSERT_LINE("\x1B[32mUser commands:\x1B[0m");\
    ASSERT_LINE("t<ind>\t\tExecute the task with the specified index");\
    ASSERT_LINE("tr<ind>\t\tKeep executing the task with the specified index until it fails");\
    ASSERT_LINE("o<ind>\t\tOpen the file link with the specified index in your code editor");\
    ASSERT_LINE("s\t\tSearch through output of the last executed task with the specified regular expression");\
    ASSERT_LINE("g<ind>\t\tDisplay output of the specified google test");\
    ASSERT_LINE("gs<ind>\t\tSearch through output of the specified google test with the specified regular expression");\
    ASSERT_LINE("gt<ind>\t\tRe-run the google test with the specified index");\
    ASSERT_LINE("gtr<ind>\tKeep re-running the google test with the specified index until it fails");\
    ASSERT_LINE("gf<ind>\t\tRun google tests of the task with the specified index with a specified --gtest_filter");\
    ASSERT_LINE("h\t\tDisplay list of user commands")
#define ASSERT_RUNNING_TASK(TASK_NAME) ASSERT_LINE("\x1B[35mRunning \"" TASK_NAME "\"\x1B[0m")
#define ASSERT_RUNNING_PRE_TASK(TASK_NAME) ASSERT_LINE("\x1B[34mRunning \"" TASK_NAME "\"...\x1B[0m")
#define ASSERT_TASK_COMPLETE(TASK_NAME) ASSERT_LINE("\x1B[35m'" TASK_NAME "' complete: return code: 0\x1B[0m")
#define ASSERT_TASK_FAILED(TASK_NAME, RETURN_CODE) ASSERT_LINE("\x1B[31m'" TASK_NAME "' failed: return code: " RETURN_CODE "\x1B[0m")
#define ASSERT_HELLO_WORLD_TASK_RAN()\
    ASSERT_RUNNING_TASK("hello world");\
    ASSERT_LINE("hello world");\
    ASSERT_TASK_COMPLETE("hello world")
#define ASSERT_CDT_RESTARTED()\
    ASSERT_LINE("\x1B[35mRestarting program...\x1B[0m");\
    ASSERT_HELP_PROMPT_DISPLAYED();\
    ASSERT_TASK_LIST_DISPLAYED()
#define ASSERT_OUTPUT_WITH_LINKS_DISPLAYED()\
    ASSERT_LINE("\x1B[35m[o1] /a/b/c:10\x1B[0m");\
    ASSERT_LINE("some random data");\
    ASSERT_LINE("\x1B[35m[o2] /d/e/f:15:32\x1B[0m something")
#define ASSERT_LAST_EXEC_OUTPUT_DISPLAYED() ASSERT_LINE("\x1B[32mLast execution output:\x1B[0m")
#define ASSERT_NO_FILE_LINKS_IN_OUTPUT_DISPLAYED() ASSERT_LINE("\x1B[32mNo file links in the output\x1B[0m")
#define ASSERT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS()\
    run_cmd("o0");\
    ASSERT_LAST_EXEC_OUTPUT_DISPLAYED();\
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();\
    run_cmd("o99");\
    ASSERT_LAST_EXEC_OUTPUT_DISPLAYED();\
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();\
    run_cmd("o");\
    ASSERT_LAST_EXEC_OUTPUT_DISPLAYED();\
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();\
    ASSERT_CMD_OUT("") // assert that we didn't attempt to open some weird link that does not exist
#define ASSERT_NON_GTEST_BINARY_EXECUTED(TASK_NAME, NON_GTEST_BINARY, RETURN_CODE)\
    ASSERT_RUNNING_TASK(TASK_NAME);\
    ASSERT_LINE("\x1B[31m'" NON_GTEST_BINARY "' is not a google test executable\x1B[0m");\
    ASSERT_TASK_FAILED(TASK_NAME, RETURN_CODE)
#define ASSERT_GTEST_TASK_FINISHED_PREMATURELY(TASK_NAME, FAILED_TEST, RETURN_CODE)\
    ASSERT_LINE("\x1B[31mTests have finished prematurely\x1B[0m");\
    ASSERT_LINE("\x1B[31mFailed tests:\x1B[0m");\
    ASSERT_LINE("1 \"" FAILED_TEST "\" ");\
    ASSERT_LINE("\x1B[31mTests failed: 1 of 2 (50%) \x1B[0m");\
    ASSERT_LINE("\x1B[31m\"" FAILED_TEST "\" output:\x1B[0m");\
    ASSERT_TASK_FAILED(TASK_NAME, RETURN_CODE)
#define ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED()\
    ASSERT_LINE("\x1B[32mLast executed tests (X ms total):\x1B[0m");\
    ASSERT_LINE("1 \"test_suit_1.test1\" (X ms)");\
    ASSERT_LINE("2 \"test_suit_1.test2\" (X ms)");\
    ASSERT_LINE("3 \"test_suit_2.test1\" (X ms)")
#define ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED()\
    ASSERT_LINE("\x1B[31mFailed tests:\x1B[0m");\
    ASSERT_LINE("1 \"failed_test_suit_1.test1\" (X ms)");\
    ASSERT_LINE("2 \"failed_test_suit_2.test1\" (X ms)");\
    ASSERT_LINE("\x1B[31mTests failed: 2 of 3 (66%) (X ms total)\x1B[0m")
#define ASSERT_SINGLE_FAILED_GTEST_TEST_DISPLAYED()\
    ASSERT_LINE("\x1B[31mFailed tests:\x1B[0m");\
    ASSERT_LINE("1 \"failed_test_suit_1.test1\" (X ms)");\
    ASSERT_LINE("\x1B[31mTests failed: 1 of 2 (50%) (X ms total)\x1B[0m");\
    ASSERT_LINE("\x1B[31m\"failed_test_suit_1.test1\" output:\x1B[0m");\
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();\
    ASSERT_GTEST_FAILURE_REASON_DISPLAYED();\
    ASSERT_TASK_FAILED("gtest with single failed test", "1")
#define ASSERT_GTEST_FAILURE_REASON_DISPLAYED()\
    ASSERT_LINE("unknown file: Failure");\
    ASSERT_LINE("C++ exception with description \"\" thrown in the test body.")
#define ASSERT_SPORIDICALLY_FAILING_TEST_FAILED()\
    ASSERT_LINE("\x1B[31mFailed tests:\x1B[0m");\
    ASSERT_LINE("1 \"sporadically_failing_tests.test1\" (X ms)");\
    ASSERT_LINE("\x1B[31mTests failed: 1 of 1 (100%) (X ms total)\x1B[0m");\
    ASSERT_LINE("\x1B[31m\"sporadically_failing_tests.test1\" output:\x1B[0m");\
    ASSERT_GTEST_FAILURE_REASON_DISPLAYED()
#define OPEN_FILE_LINKS_AND_ASSERT_LINKS_OPENED_IN_EDITOR()\
    run_cmd("o1");\
    run_cmd("o2");\
    run_cmd("h");\
    ASSERT_HELP_DISPLAYED();\
    ASSERT_CMD_OUT("/a/b/c:10\n/d/e/f:15:32\n")
#define ASSERT_SEARCH_COMMAND_HANDLES_INVALID_REGULAR_EXPRESSION(CMD)\
    run_cmd(CMD);\
    run_cmd("[");\
    ASSERT_LINE("\x1B[31mInvalid regular expression '[': The expression contained mismatched [ and ].")
#define ASSERT_NO_SEARCH_RESULTS() ASSERT_LINE("\x1B[32mNo matches found\x1B[0m")
#define ASSERT_SEARCH_COMMAND_HANDLES_NO_RESULTS(CMD)\
    run_cmd(CMD);\
    run_cmd("non\\-existent");\
    ASSERT_NO_SEARCH_RESULTS()

TEST_F(cdt_test, start_and_view_tasks) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
}

TEST_F(cdt_test, fail_to_start_due_to_user_config_not_being_json) {
    using namespace std::string_literals;
    run_cdt("test-tasks.json", "non-json-config");
    ASSERT_LINE("\x1B[31mFailed to parse "s + to_absolute_user_config_path("non-json-config") + ": [json.exception.parse_error.101] parse error at line 1, column 1: syntax error while parsing value - invalid literal; last read: 'p'");
}

TEST_F(cdt_test, fail_to_start_due_to_user_config_having_open_in_editor_command_in_incorrect_format) {
    run_cdt("test-tasks.json", "incorrect-open-in-editor-command");
    ASSERT_LINE(is_invalid_error_message(to_absolute_user_config_path("incorrect-open-in-editor-command")));
    ASSERT_LINE("'open_in_editor_command': must be a string in format: 'notepad++ {}', where {} will be replaced with a file name");
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_specified) {
    const std::vector<std::string> args = {cwd / BINARY_NAME};
    proc = std::make_unique<TinyProcessLib::Process>(args, "", write_to(output, stdout_buffer), write_to(output, stderr_buffer), true);
    ASSERT_LINE("\x1B[31musage: cpp-dev-tools tasks.json");
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_existing) {
    using namespace std::string_literals;
    run_cdt("non-existent-config.json", "no-config");
    ASSERT_LINE("\x1B[31m"s + to_absolute_tasks_config_path("non-existent-config.json") + " does not exist");
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_being_json) {
    using namespace std::string_literals;
    run_cdt("non-json-config.json", "no-config");
    ASSERT_LINE("\x1B[31mFailed to parse "s + to_absolute_tasks_config_path("non-json-config.json") + ": [json.exception.parse_error.101] parse error at line 1, column 1: syntax error while parsing value - invalid literal; last read: 'p'");
}

TEST_F(cdt_test, fail_to_start_due_to_cdt_tasks_not_being_specified_in_config) {
    run_cdt("cdt_tasks-not-specified.json", "no-config");
    ASSERT_LINE(is_invalid_error_message(to_absolute_tasks_config_path("cdt_tasks-not-specified.json")));
    ASSERT_LINE("'cdt_tasks': must be an array of task objects");
}

TEST_F(cdt_test, fail_to_start_due_to_cdt_tasks_not_being_array_of_objects) {
    run_cdt("cdt_tasks-not-array.json", "no-config");
    ASSERT_LINE(is_invalid_error_message(to_absolute_tasks_config_path("cdt_tasks-not-array.json")));
    ASSERT_LINE("'cdt_tasks': must be an array of task objects");
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_having_errors) {
    run_cdt("config-with-errors.json", "no-config");
    ASSERT_LINE(is_invalid_error_message(to_absolute_tasks_config_path("config-with-errors.json")));
    ASSERT_LINE("task #1: 'name': must be a string");
    ASSERT_LINE("task #2: 'command': must be a string");
    ASSERT_LINE("task #2: 'pre_tasks': must be an array of other task names");
    ASSERT_LINE("task #3: references task 'non-existent-task' that does not exist");
    ASSERT_LINE("task 'cycle-1' has a circular dependency in it's 'pre_tasks':");
    ASSERT_LINE("cycle-1 -> cycle-2 -> cycle-3 -> cycle-1");
    ASSERT_LINE("task 'cycle-2' has a circular dependency in it's 'pre_tasks':");
    ASSERT_LINE("cycle-2 -> cycle-3 -> cycle-1 -> cycle-2");
    ASSERT_LINE("task 'cycle-3' has a circular dependency in it's 'pre_tasks':");
    ASSERT_LINE("cycle-3 -> cycle-1 -> cycle-2 -> cycle-3");
}

TEST_F(cdt_test, start_and_display_help) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    // Display help on unknown command
    run_cmd("zz");
    ASSERT_HELP_DISPLAYED();
    // Display help on explicit command
    run_cmd("h");
    ASSERT_HELP_DISPLAYED();
}

TEST_F(cdt_test, start_and_display_list_of_tasks_on_task_command) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    // Display tasks list on no index
    run_cmd("t");
    ASSERT_TASK_LIST_DISPLAYED();
    // Display tasks list on non-existent task specified
    run_cmd("t0");
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t999");
    ASSERT_TASK_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_and_execute_single_task) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t1");
    ASSERT_HELLO_WORLD_TASK_RAN();
    ASSERT_CMD_OUT("hello world\n");
}

TEST_F(cdt_test, start_and_execute_task_with_pre_tasks_with_pre_tasks) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t2");
    ASSERT_RUNNING_PRE_TASK("pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_PRE_TASK("pre task 2");
    ASSERT_RUNNING_TASK("primary task");
    ASSERT_LINE("primary task");
    ASSERT_TASK_COMPLETE("primary task");
    ASSERT_CMD_OUT("pre task 1\npre pre task 1\npre pre task 2\npre task 2\nprimary task\n");
}

TEST_F(cdt_test, start_and_fail_primary_task) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t7");
    ASSERT_RUNNING_TASK("fail pre task");
    ASSERT_TASK_FAILED("fail pre task", "15");
}

TEST_F(cdt_test, start_and_fail_one_of_pre_tasks) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t8");
    ASSERT_RUNNING_PRE_TASK("pre task 1");
    ASSERT_RUNNING_PRE_TASK("fail pre task");
    ASSERT_TASK_FAILED("fail pre task", "15");
    ASSERT_CMD_OUT("pre task 1\n");
}

TEST_F(cdt_test, start_execute_task_and_abort_it) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t9");
    ASSERT_RUNNING_TASK("long task");
    interrupt_current_task();
    ASSERT_TASK_FAILED("long task", "2");
}

TEST_F(cdt_test, start_and_exit) {
    using namespace std::chrono_literals;
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    proc->signal(SIGINT);
    int exit_code = 0;
    for (auto i = 0; i < 10; i++) {
        if (proc->try_get_exit_status(exit_code)) {
            break;
        }
        std::this_thread::sleep_for(1ms);
    }
    ASSERT_EQ(2, exit_code);
}

TEST_F(cdt_test, start_and_change_cwd_to_tasks_configs_directory) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t10");
    ASSERT_RUNNING_TASK("current working directory");
    ASSERT_LINE(test_configs_dir.string());
    ASSERT_TASK_COMPLETE("current working directory");
}

TEST_F(cdt_test, start_execute_single_task_and_repeate_the_last_command_on_enter) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t1");
    ASSERT_HELLO_WORLD_TASK_RAN();
    run_cmd("");
    ASSERT_HELLO_WORLD_TASK_RAN();
}

TEST_F(cdt_test, start_and_execute_restart_task) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t11");
    ASSERT_CDT_RESTARTED();
    // Last user command should persist through restart, so that pressing enter right now will trigger restart again
    run_cmd("");
    ASSERT_CDT_RESTARTED();
}

TEST_F(cdt_test, start_execute_task_and_open_links_from_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t12");
    ASSERT_RUNNING_TASK("primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_COMPLETE("primary task with links");
    OPEN_FILE_LINKS_AND_ASSERT_LINKS_OPENED_IN_EDITOR();
}

TEST_F(cdt_test, start_fail_to_execute_task_with_links_and_open_links_from_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t13");
    ASSERT_RUNNING_TASK("fail primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_FAILED("fail primary task with links", "32");
    OPEN_FILE_LINKS_AND_ASSERT_LINKS_OPENED_IN_EDITOR();
}

TEST_F(cdt_test, start_fail_to_execute_pre_task_of_task_and_open_links_from_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t14");
    ASSERT_RUNNING_PRE_TASK("fail primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_FAILED("fail primary task with links", "32");
    OPEN_FILE_LINKS_AND_ASSERT_LINKS_OPENED_IN_EDITOR();
}

TEST_F(cdt_test, start_execute_task_with_links_in_output_attempt_to_open_non_existent_link_and_view_task_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t12");
    ASSERT_RUNNING_TASK("primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_COMPLETE("primary task with links");
    ASSERT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_fail_to_execute_task_with_links_attempt_to_open_non_existent_link_and_view_task_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t13");
    ASSERT_RUNNING_TASK("fail primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_FAILED("fail primary task with links", "32");
    ASSERT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_fail_to_execute_pre_task_of_task_attempt_to_open_non_existent_link_and_view_task_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t14");
    ASSERT_RUNNING_PRE_TASK("fail primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_FAILED("fail primary task with links", "32");
    ASSERT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_attempt_to_open_non_existent_link_and_view_task_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("o1");
    ASSERT_NO_FILE_LINKS_IN_OUTPUT_DISPLAYED();
    run_cmd("o");
    ASSERT_NO_FILE_LINKS_IN_OUTPUT_DISPLAYED();
}

TEST_F(cdt_test, start_attempt_to_open_link_while_open_in_editor_command_is_not_specified_and_see_error) {
    using namespace std::string_literals;
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("o");
    ASSERT_LINE("\x1B[31m'open_in_editor_command' is not specified in \""s + to_absolute_user_config_path("no-config") + "\"\x1B[0m");
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_no_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t15");
    ASSERT_RUNNING_TASK("gtest with no tests");
    ASSERT_LINE("\x1B[32mSuccessfully executed 0 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with no tests");
}

TEST_F(cdt_test, start_attempt_to_execute_gtest_task_with_non_existent_binary_and_fail) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t16");
    ASSERT_NON_GTEST_BINARY_EXECUTED("gtest with non-existent binary", "./non-existent-binary", "127");
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_non_gtest_binary) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t17");
    ASSERT_NON_GTEST_BINARY_EXECUTED("gtest with non-gtest binary", "echo hello world!", "0");
}

TEST_F(cdt_test, start_execute_gtest_task_with_non_gtest_binary_that_does_not_finish_and_abort_it_manually) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t18");
    ASSERT_RUNNING_TASK("gtest with long non-gtest binary");
    interrupt_current_task();
    ASSERT_LINE("\x1B[31m'sleep 20' is not a google test executable\x1B[0m");
    ASSERT_TASK_FAILED("gtest with long non-gtest binary", "2");
}

TEST_F(cdt_test, start_and_execute_gtest_task_that_exits_with_0_exit_code_in_the_middle) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t19");
    ASSERT_RUNNING_TASK("gtest exit in the middle with 0 exit code");
    ASSERT_GTEST_TASK_FINISHED_PREMATURELY("gtest exit in the middle with 0 exit code", "exit_tests.exit_with_0", "0");
}

TEST_F(cdt_test, start_and_execute_gtest_task_that_exits_with_1_exit_code_in_the_middle) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t20");
    ASSERT_RUNNING_TASK("gtest exit in the middle with 1 exit code");
    ASSERT_GTEST_TASK_FINISHED_PREMATURELY("gtest exit in the middle with 1 exit code", "exit_tests.exit_with_1", "1");
}

TEST_F(cdt_test, start_attempt_to_execute_task_with_gtest_pre_task_that_exits_with_0_exit_code_in_the_middle_and_fail) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t21");
    ASSERT_RUNNING_PRE_TASK("gtest exit in the middle with 0 exit code");
    ASSERT_GTEST_TASK_FINISHED_PREMATURELY("gtest exit in the middle with 0 exit code", "exit_tests.exit_with_0", "0");
    ASSERT_CMD_OUT("");
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_multiple_suites) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t22");
    ASSERT_RUNNING_TASK("gtest with multiple test suites");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with multiple test suites");
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_multiple_suites_each_having_failed_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t23");
    ASSERT_RUNNING_TASK("gtest with multiple failed test suites");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    ASSERT_TASK_FAILED("gtest with multiple failed test suites", "1");
}

TEST_F(cdt_test, start_and_execute_task_with_gtest_pre_task_with_multiple_suites) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t24");
    ASSERT_RUNNING_PRE_TASK("gtest with multiple test suites");
    ASSERT_RUNNING_TASK("task woth gtest pre task with multiple test suites");
    ASSERT_LINE("hello world");
    ASSERT_TASK_COMPLETE("task woth gtest pre task with multiple test suites");
    ASSERT_CMD_OUT("hello world\n");
}

TEST_F(cdt_test, start_and_execute_task_with_gtest_pre_task_with_multiple_suites_each_having_failed_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t25");
    ASSERT_RUNNING_PRE_TASK("gtest with multiple failed test suites");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    ASSERT_TASK_FAILED("gtest with multiple failed test suites", "1");
}

TEST_F(cdt_test, start_execute_gtest_task_with_long_test_and_abort_it) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t26");
    ASSERT_RUNNING_TASK("gtest with long test");
    interrupt_current_task();
    ASSERT_GTEST_TASK_FINISHED_PREMATURELY("gtest with long test", "long_tests.test1", "2");
}

TEST_F(cdt_test, start_execute_gtest_task_with_failed_tests_and_long_test_and_abort_it) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t27");
    ASSERT_RUNNING_TASK("gtest with long test and failures");
    interrupt_current_task();
    ASSERT_LINE("\x1B[31mTests have finished prematurely\x1B[0m");
    ASSERT_LINE("\x1B[31mFailed tests:\x1B[0m");
    ASSERT_LINE("1 \"failed_test_suit_1.test1\" (X ms)");
    ASSERT_LINE("2 \"long_tests.test1\" ");
    ASSERT_LINE("\x1B[31mTests failed: 2 of 2 (100%) \x1B[0m");
    ASSERT_TASK_FAILED("gtest with long test and failures", "2");
}

TEST_F(cdt_test, start_repeatedly_execute_task_until_it_fails) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("tr28");
    // First execution should succeed
    ASSERT_RUNNING_TASK("gtest with sporadically failing test");
    ASSERT_LINE("\x1B[32mSuccessfully executed 1 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with sporadically failing test");
    // Second execution should succeed
    ASSERT_RUNNING_TASK("gtest with sporadically failing test");
    ASSERT_LINE("\x1B[32mSuccessfully executed 1 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with sporadically failing test");
    // Third execution should fail
    ASSERT_RUNNING_TASK("gtest with sporadically failing test");
    ASSERT_SPORIDICALLY_FAILING_TEST_FAILED();
    ASSERT_TASK_FAILED("gtest with sporadically failing test", "1");
}

TEST_F(cdt_test, start_repeatedly_execute_task_until_one_of_its_pre_tasks_fails) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("tr8");
    ASSERT_RUNNING_PRE_TASK("pre task 1");
    ASSERT_RUNNING_PRE_TASK("fail pre task");
    ASSERT_TASK_FAILED("fail pre task", "15");
    ASSERT_CMD_OUT("pre task 1\n");
}

TEST_F(cdt_test, start_repeatedly_execute_long_task_and_abort_it) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("tr9");
    ASSERT_RUNNING_TASK("long task");
    interrupt_current_task();
    ASSERT_TASK_FAILED("long task", "2");
}

TEST_F(cdt_test, start_attempt_to_view_gtest_tests_but_see_no_tests_have_been_executed_yet) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("g");
    ASSERT_LINE("\x1B[32mNo google tests have been executed yet.\x1B[0m");
}

TEST_F(cdt_test, start_execute_gtest_task_try_to_view_gtest_test_output_with_index_out_of_range_and_see_all_tests_list) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t22");
    ASSERT_RUNNING_TASK("gtest with multiple test suites");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with multiple test suites");
    run_cmd("g0");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("g99");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("g");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_execute_gtest_task_view_gtest_task_output_with_file_links_highlighted_in_the_output_and_open_links) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t22");
    ASSERT_RUNNING_TASK("gtest with multiple test suites");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with multiple test suites");
    run_cmd("g1");
    ASSERT_LINE("\x1B[32m\"test_suit_1.test1\" output:\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    OPEN_FILE_LINKS_AND_ASSERT_LINKS_OPENED_IN_EDITOR();
}

TEST_F(cdt_test, start_execute_gtest_task_fail_try_to_view_gtest_test_output_with_index_out_of_range_and_see_failed_tests_list) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t23");
    ASSERT_RUNNING_TASK("gtest with multiple failed test suites");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    ASSERT_TASK_FAILED("gtest with multiple failed test suites", "1");
    run_cmd("g0");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("g99");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("g");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_execute_gtest_task_fail_view_gtest_test_output_with_file_links_highlighted_in_the_output_and_open_links) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t23");
    ASSERT_RUNNING_TASK("gtest with multiple failed test suites");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    ASSERT_TASK_FAILED("gtest with multiple failed test suites", "1");
    run_cmd("g1");
    ASSERT_LINE("\x1B[31m\"failed_test_suit_1.test1\" output:\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_GTEST_FAILURE_REASON_DISPLAYED();
    run_cmd("g1");
    ASSERT_LINE("\x1B[31m\"failed_test_suit_1.test1\" output:\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_GTEST_FAILURE_REASON_DISPLAYED();
    OPEN_FILE_LINKS_AND_ASSERT_LINKS_OPENED_IN_EDITOR();
}

TEST_F(cdt_test, start_execute_gtest_task_fail_one_of_the_tests_and_view_automatically_displayed_test_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t29");
    ASSERT_RUNNING_TASK("gtest with single failed test");
    ASSERT_SINGLE_FAILED_GTEST_TEST_DISPLAYED();
}

TEST_F(cdt_test, start_execute_task_with_gtest_pre_task_fail_one_of_the_tests_and_view_automatically_displayed_test_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t30");
    ASSERT_RUNNING_PRE_TASK("gtest with single failed test");
    ASSERT_SINGLE_FAILED_GTEST_TEST_DISPLAYED();
}

TEST_F(cdt_test, start_attempt_to_rerun_gtest_when_no_tests_have_been_executed_yet) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("gt");
    ASSERT_LINE("\x1B[32mNo google tests have been executed yet.\x1B[0m");
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_attempt_to_rerun_test_that_does_not_exist_and_view_list_of_failed_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t32");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple failed test suites and pre tasks");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    ASSERT_TASK_FAILED("gtest with multiple failed test suites and pre tasks", "1");
    ASSERT_CMD_OUT(TWO_PRE_TASKS_OUTPUT);
    run_cmd("gt");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gt0");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gt99");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_and_rerun_failed_test) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t32");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple failed test suites and pre tasks");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    ASSERT_TASK_FAILED("gtest with multiple failed test suites and pre tasks", "1");
    run_cmd("gt1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("failed_test_suit_1.test1");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_GTEST_FAILURE_REASON_DISPLAYED();
    ASSERT_TASK_FAILED("failed_test_suit_1.test1", "1");
    run_cmd("gt2");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("failed_test_suit_2.test1");
    ASSERT_GTEST_FAILURE_REASON_DISPLAYED();
    ASSERT_TASK_FAILED("failed_test_suit_2.test1", "1");
    ASSERT_CMD_OUT(TWO_PRE_TASKS_OUTPUT + TWO_PRE_TASKS_OUTPUT + TWO_PRE_TASKS_OUTPUT);
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_rerun_test_that_does_not_exist_and_view_list_of_all_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t31");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple test suites and pre tasks");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with multiple test suites and pre tasks");
    ASSERT_CMD_OUT(TWO_PRE_TASKS_OUTPUT);
    run_cmd("gt");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gt0");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gt99");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_and_rerun_one_of_test) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t31");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple test suites and pre tasks");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with multiple test suites and pre tasks");
    run_cmd("gt1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("test_suit_1.test1");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_COMPLETE("test_suit_1.test1");
    run_cmd("gt2");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("test_suit_1.test2");
    ASSERT_TASK_COMPLETE("test_suit_1.test2");
    ASSERT_CMD_OUT(TWO_PRE_TASKS_OUTPUT + TWO_PRE_TASKS_OUTPUT + TWO_PRE_TASKS_OUTPUT);
}

TEST_F(cdt_test, start_repeatedly_execute_gtest_task_with_pre_tasks_until_it_fails_and_repeatedly_rerun_failed_test_until_it_fails) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("tr33");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    // First execution should succeed
    ASSERT_RUNNING_TASK("gtest with sporadically failing test with pre tasks");
    ASSERT_LINE("\x1B[32mSuccessfully executed 1 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with sporadically failing test with pre tasks");
    // Second execution should succeed
    ASSERT_RUNNING_TASK("gtest with sporadically failing test with pre tasks");
    ASSERT_LINE("\x1B[32mSuccessfully executed 1 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with sporadically failing test with pre tasks");
    // Third execution should fail
    ASSERT_RUNNING_TASK("gtest with sporadically failing test with pre tasks");
    ASSERT_SPORIDICALLY_FAILING_TEST_FAILED();
    ASSERT_TASK_FAILED("gtest with sporadically failing test with pre tasks", "1");
    run_cmd("gtr1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    // First re-run should succeed
    ASSERT_RUNNING_TASK("sporadically_failing_tests.test1");
    ASSERT_TASK_COMPLETE("sporadically_failing_tests.test1");
    // Second re-run should succeed
    ASSERT_RUNNING_TASK("sporadically_failing_tests.test1");
    ASSERT_TASK_COMPLETE("sporadically_failing_tests.test1");
    // Third re-run should fail
    ASSERT_RUNNING_TASK("sporadically_failing_tests.test1");
    ASSERT_GTEST_FAILURE_REASON_DISPLAYED();
    ASSERT_TASK_FAILED("sporadically_failing_tests.test1", "1");
    ASSERT_CMD_OUT(TWO_PRE_TASKS_OUTPUT + TWO_PRE_TASKS_OUTPUT);
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_repeatedly_rerun_one_of_the_tests_and_fail_due_to_failed_pre_task) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    // Run the sporadically failing task once, so that it will fail after the next execution.
    run_cmd("t28");
    ASSERT_RUNNING_TASK("gtest with sporadically failing test");
    ASSERT_LINE("\x1B[32mSuccessfully executed 1 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with sporadically failing test");
    run_cmd("t34");
    ASSERT_RUNNING_PRE_TASK("gtest with sporadically failing test");
    ASSERT_RUNNING_TASK("gtest task with sporadically failing pre task");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest task with sporadically failing pre task");
    run_cmd("gtr1");
    ASSERT_RUNNING_PRE_TASK("gtest with sporadically failing test");
    ASSERT_SPORIDICALLY_FAILING_TEST_FAILED();
    ASSERT_TASK_FAILED("gtest with sporadically failing test", "1");
}

TEST_F(cdt_test, start_execute_long_gtest_task_with_pre_tasks_abort_it_repeatedly_rerun_failed_test_and_abort_it_again) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t35");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with long test with pre tasks");
    interrupt_current_task();
    ASSERT_GTEST_TASK_FINISHED_PREMATURELY("gtest with long test with pre tasks", "long_tests.test1", "2");
    run_cmd("gtr1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("long_tests.test1");
    interrupt_current_task();
    ASSERT_TASK_FAILED("long_tests.test1", "2");
}

TEST_F(cdt_test, start_attempt_to_repeatedly_rerun_gtest_when_no_tests_have_been_executed_yet) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("gtr");
    ASSERT_LINE("\x1B[32mNo google tests have been executed yet.\x1B[0m");
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_repeatedly_rerun_test_that_does_not_exist_and_view_list_of_all_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t31");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple test suites and pre tasks");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with multiple test suites and pre tasks");
    ASSERT_CMD_OUT(TWO_PRE_TASKS_OUTPUT);
    run_cmd("gtr");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gtr0");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gtr99");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_and_create_example_user_config) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_EQ(DEFAULT_USER_CONFIG_CONTENT, read_file(to_absolute_user_config_path("no-config")));
}

TEST_F(cdt_test, start_and_not_override_existing_user_config) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_NE(DEFAULT_USER_CONFIG_CONTENT, read_file(to_absolute_user_config_path("test-config")));
}

TEST_F(cdt_test, start_attempt_to_execute_google_tests_with_filter_targeting_task_that_does_not_exist_and_view_list_of_task) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("gf");
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("gf0");
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("gf99");
    ASSERT_TASK_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_gtest_filter) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("gf19");
    run_cmd("normal_tests.*");
    ASSERT_RUNNING_TASK("normal_tests.*");
    ASSERT_LINE("\x1B[32mSuccessfully executed 1 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("normal_tests.*");
}

TEST_F(cdt_test, start_attempt_to_search_execution_output_but_fail_due_to_no_task_being_executed_before_it) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("s");
    ASSERT_LINE("\x1B[32mNo task has been executed yet\x1B[0m");
}

TEST_F(cdt_test, start_execute_task_attempt_to_search_its_output_with_invalid_regex_and_fail) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t12");
    ASSERT_RUNNING_TASK("primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_COMPLETE("primary task with links");
    ASSERT_SEARCH_COMMAND_HANDLES_INVALID_REGULAR_EXPRESSION("s");
}

TEST_F(cdt_test, start_execute_task_search_its_output_and_find_no_results) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t12");
    ASSERT_RUNNING_TASK("primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_COMPLETE("primary task with links");
    ASSERT_SEARCH_COMMAND_HANDLES_NO_RESULTS("s");
}

TEST_F(cdt_test, start_execute_task_search_its_output_and_find_results) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t12");
    ASSERT_RUNNING_TASK("primary task with links");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_TASK_COMPLETE("primary task with links");
    run_cmd("s");
    run_cmd("(some|data)");
    ASSERT_LINE("\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m");
    ASSERT_LINE("\x1B[35m3:\x1B[0m\x1B[35m[o2] /d/e/f:15:32\x1B[0m \x1B[32msome\x1B[0mthing");
}

TEST_F(cdt_test, start_attempt_to_search_gtest_output_when_no_tests_have_been_executed_yet) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("gs");
    ASSERT_LINE("\x1B[32mNo google tests have been executed yet.\x1B[0m");
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_attempt_to_search_output_of_test_that_does_not_exist_and_view_list_of_failed_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t32");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple failed test suites and pre tasks");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    ASSERT_TASK_FAILED("gtest with multiple failed test suites and pre tasks", "1");
    ASSERT_CMD_OUT(TWO_PRE_TASKS_OUTPUT);
    run_cmd("gs");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gs0");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gs99");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_and_search_output_of_the_test) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t32");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple failed test suites and pre tasks");
    ASSERT_FAILED_GTEST_TESTS_LIST_DISPLAYED();
    ASSERT_TASK_FAILED("gtest with multiple failed test suites and pre tasks", "1");
    ASSERT_SEARCH_COMMAND_HANDLES_INVALID_REGULAR_EXPRESSION("gs1");
    ASSERT_SEARCH_COMMAND_HANDLES_NO_RESULTS("gs1");
    run_cmd("gs1");
    run_cmd("(C\\+\\+|with description|Failure)");
    ASSERT_LINE("\x1B[35m4:\x1B[0munknown file: \x1B[32mFailure\x1B[0m");
    ASSERT_LINE("\x1B[35m5:\x1B[0m\x1B[32mC++\x1B[0m exception \x1B[32mwith description\x1B[0m \"\" thrown in the test body.");
    run_cmd("gs2");
    run_cmd("(C\\+\\+|with description|Failure)");
    ASSERT_LINE("\x1B[35m1:\x1B[0munknown file: \x1B[32mFailure\x1B[0m");
    ASSERT_LINE("\x1B[35m2:\x1B[0m\x1B[32mC++\x1B[0m exception \x1B[32mwith description\x1B[0m \"\" thrown in the test body.");
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_search_output_of_test_that_does_not_exist_and_view_list_of_all_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t31");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple test suites and pre tasks");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with multiple test suites and pre tasks");
    ASSERT_CMD_OUT(TWO_PRE_TASKS_OUTPUT);
    run_cmd("gs");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gs0");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
    run_cmd("gs99");
    ASSERT_SUCCESS_GTEST_TESTS_LIST_DISPLAYED();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_and_search_output_of_one_of_the_tests) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t31");
    ASSERT_RUNNING_PRE_TASK("pre pre task 1");
    ASSERT_RUNNING_PRE_TASK("pre pre task 2");
    ASSERT_RUNNING_TASK("gtest with multiple test suites and pre tasks");
    ASSERT_LINE("\x1B[32mSuccessfully executed 3 tests (X ms total)\x1B[0m");
    ASSERT_TASK_COMPLETE("gtest with multiple test suites and pre tasks");
    ASSERT_SEARCH_COMMAND_HANDLES_INVALID_REGULAR_EXPRESSION("gs1");
    ASSERT_SEARCH_COMMAND_HANDLES_NO_RESULTS("gs1");
    run_cmd("gs1");
    run_cmd("(some|data)");
    ASSERT_LINE("\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m");
    ASSERT_LINE("\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing");
    run_cmd("gs2");
    run_cmd("(some|data)");
    ASSERT_NO_SEARCH_RESULTS();
}
