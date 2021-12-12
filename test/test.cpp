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
    const std::string CDT_PREFIX = "\x1B[32m(cdt) \x1B[0m";
    const std::filesystem::path cwd = std::filesystem::path(BINARY_DIR);
    const std::filesystem::path test_configs_dir = std::filesystem::path(TEST_CONFIGS_DIR);
    const std::filesystem::path test_homes_dir = std::filesystem::path(TEST_HOMES_DIR);
    const std::filesystem::path cmd_out_file = cwd / "cmd-test-output.txt";

    moodycamel::BlockingConcurrentQueue<std::string> output;
    std::string stdout_buffer, stderr_buffer;
    std::unique_ptr<TinyProcessLib::Process> proc;

    virtual void SetUp() override {
        std::filesystem::remove(cmd_out_file);
    }
    virtual void TearDown() override {
        if (proc) {
            proc->kill();
        }
        std::filesystem::remove(cmd_out_file);
    }
    void run_cdt(const std::string& config_name, const std::string& home_dir) {
        const std::vector<std::string> args = {cwd / BINARY_NAME, test_configs_dir / config_name};
        const std::unordered_map<std::string, std::string> env = {
            {"HOME", test_homes_dir / home_dir},
            {"OUTPUT_FILE", cmd_out_file}
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
            EXPECT_EQ("", get_out_line());
        }
    }
    std::string get_out_line() {
        std::string msg;
        output.wait_dequeue(msg);
        // Ignore "(cdt) " output when waiting for a command
        const auto pos = msg.rfind(CDT_PREFIX);
        if (pos != std::string::npos) {
            msg = msg.substr(pos + CDT_PREFIX.size());
        }
        return msg;
    }
    std::string read_cmd_test_output() {
        std::ifstream file(cmd_out_file);
        if (!file) return "";
        file >> std::noskipws;
        return std::string(std::istream_iterator<char>(file), std::istream_iterator<char>());
    }
};

#define ASSERT_LINE(LINE) ASSERT_EQ(LINE, get_out_line())
#define ASSERT_CMD_OUT(OUT) ASSERT_EQ(OUT, read_cmd_test_output())
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
    ASSERT_LINE("14 \"primary task with fail pre task with links\"")
#define ASSERT_HELP_DISPLAYED()\
    ASSERT_LINE("\x1B[32mUser commands:\x1B[0m");\
    ASSERT_LINE("t<ind>\t\tExecute the task with the specified index");\
    ASSERT_LINE("o<ind>\t\tOpen the file link with the specified index in your code editor");\
    ASSERT_LINE("h\t\tDisplay list of user commands")
#define ASSERT_HELLO_WORLD_TASK_RAN()\
    ASSERT_LINE("\x1B[35mRunning \"hello world\"\x1B[0m");\
    ASSERT_LINE("hello world");\
    ASSERT_LINE("\x1B[35m'hello world' complete: return code: 0\x1B[0m")
#define ASSERT_CDT_RESTARTED()\
    ASSERT_LINE("\x1B[35mRunning \"restart\"\x1B[0m");\
    ASSERT_LINE("\x1B[35mRestarting program...\x1B[0m");\
    ASSERT_HELP_PROMPT_DISPLAYED();\
    ASSERT_TASK_LIST_DISPLAYED()
#define ASSERT_OUTPUT_WITH_LINKS_DISPLAYED()\
    ASSERT_LINE("\x1B[35m[o1] /a/b/c:10\x1B[0m");\
    ASSERT_LINE("some random data");\
    ASSERT_LINE("\x1B[35m[o2] /d/e/f:15:32\x1B[0m something")
#define ASSERT_FILE_LINKS_OPENED_IN_EDITOR() ASSERT_CMD_OUT("/a/b/c:10\n/d/e/f:15:32\n")
#define ASSERT_LAST_TASK_OUTPUT_DISPLAYED() ASSERT_LINE("\x1B[32mLast task output:\x1B[0m")
#define ASSERT_NO_FILE_LINKS_IN_OUTPUT_DISPLAYED() ASSERT_LINE("\x1B[32mNo file links in the output\x1B[0m")
#define ASSERT_LAST_TASK_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS()\
    run_cmd("o0");\
    ASSERT_LAST_TASK_OUTPUT_DISPLAYED();\
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();\
    run_cmd("o99");\
    ASSERT_LAST_TASK_OUTPUT_DISPLAYED();\
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();\
    run_cmd("o");\
    ASSERT_LAST_TASK_OUTPUT_DISPLAYED();\
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();\
    ASSERT_CMD_OUT("") // assert that we didn't attempt to open some weird link that does not exist
#define WAIT_FOR_LAST_CMD_TO_COMPLETE()\
    run_cmd("h");\
    ASSERT_HELP_DISPLAYED()

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
    ASSERT_LINE("\x1B[34mRunning \"pre task 1\"...\x1B[0m");
    ASSERT_LINE("\x1B[34mRunning \"pre pre task 1\"...\x1B[0m");
    ASSERT_LINE("\x1B[34mRunning \"pre pre task 2\"...\x1B[0m");
    ASSERT_LINE("\x1B[34mRunning \"pre task 2\"...\x1B[0m");
    ASSERT_LINE("\x1B[35mRunning \"primary task\"\x1B[0m");
    ASSERT_LINE("primary task");
    ASSERT_LINE("\x1B[35m'primary task' complete: return code: 0\x1B[0m");
    ASSERT_CMD_OUT("pre task 1\npre pre task 1\npre pre task 2\npre task 2\nprimary task\n");
}

TEST_F(cdt_test, start_and_fail_primary_task) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t7");
    ASSERT_LINE("\x1B[35mRunning \"fail pre task\"\x1B[0m");
    ASSERT_LINE("\x1B[31m'fail pre task' failed: return code: 15\x1B[0m");
}

TEST_F(cdt_test, start_and_fail_one_of_pre_tasks) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t8");
    ASSERT_LINE("\x1B[34mRunning \"pre task 1\"...\x1B[0m");
    ASSERT_LINE("\x1B[34mRunning \"fail pre task\"...\x1B[0m");
    ASSERT_LINE("\x1B[31m'fail pre task' failed: return code: 15\x1B[0m");
    ASSERT_CMD_OUT("pre task 1\n");
}

TEST_F(cdt_test, start_execute_task_and_abort_it) {
    using namespace std::chrono_literals;
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t9");
    ASSERT_LINE("\x1B[35mRunning \"long task\"\x1B[0m");
    std::this_thread::sleep_for(10ms);
    proc->signal(SIGINT);
    ASSERT_LINE("\x1B[31m'long task' failed: return code: 2\x1B[0m");
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
    ASSERT_LINE("\x1B[35mRunning \"current working directory\"\x1B[0m");
    ASSERT_LINE(test_configs_dir.string());
    ASSERT_LINE("\x1B[35m'current working directory' complete: return code: 0\x1B[0m");
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
    ASSERT_LINE("\x1B[35mRunning \"primary task with links\"\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_LINE("\x1B[35m'primary task with links' complete: return code: 0\x1B[0m");
    run_cmd("o1");
    run_cmd("o2");
    WAIT_FOR_LAST_CMD_TO_COMPLETE();
    ASSERT_FILE_LINKS_OPENED_IN_EDITOR();
}

TEST_F(cdt_test, start_fail_to_execute_task_with_links_and_open_links_from_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t13");
    ASSERT_LINE("\x1B[35mRunning \"fail primary task with links\"\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_LINE("\x1B[31m'fail primary task with links' failed: return code: 32\x1B[0m");
    run_cmd("o1");
    run_cmd("o2");
    WAIT_FOR_LAST_CMD_TO_COMPLETE();
    ASSERT_FILE_LINKS_OPENED_IN_EDITOR();
}

TEST_F(cdt_test, start_fail_to_execute_pre_task_of_task_and_open_links_from_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t14");
    ASSERT_LINE("\x1B[34mRunning \"fail primary task with links\"...\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_LINE("\x1B[31m'fail primary task with links' failed: return code: 32\x1B[0m");
    run_cmd("o1");
    run_cmd("o2");
    WAIT_FOR_LAST_CMD_TO_COMPLETE();
    ASSERT_FILE_LINKS_OPENED_IN_EDITOR();
}

TEST_F(cdt_test, start_execute_task_with_links_in_output_attempt_to_open_non_existent_link_and_view_task_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t12");
    ASSERT_LINE("\x1B[35mRunning \"primary task with links\"\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_LINE("\x1B[35m'primary task with links' complete: return code: 0\x1B[0m");
    ASSERT_LAST_TASK_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_fail_to_execute_task_with_links_attempt_to_open_non_existent_link_and_view_task_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t13");
    ASSERT_LINE("\x1B[35mRunning \"fail primary task with links\"\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_LINE("\x1B[31m'fail primary task with links' failed: return code: 32\x1B[0m");
    ASSERT_LAST_TASK_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_fail_to_execute_pre_task_of_task_attempt_to_open_non_existent_link_and_view_task_output) {
    run_cdt("test-tasks.json", "test-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
    run_cmd("t14");
    ASSERT_LINE("\x1B[34mRunning \"fail primary task with links\"...\x1B[0m");
    ASSERT_OUTPUT_WITH_LINKS_DISPLAYED();
    ASSERT_LINE("\x1B[31m'fail primary task with links' failed: return code: 32\x1B[0m");
    ASSERT_LAST_TASK_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
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

// TODO:
// execute gtest task with no tests
// fail to execute gtest task with non-existent binary
// execute gtest task with a non-gtest binary that finishes
// execute gtest task with a non-gtest binary that does not finish and abort it
// execute gtest task with multiple suites with all tests successfull
// execute gtest task with multiple suites with failed tests in each suite
// execute task with gtest pre task with multiple suites with all tests successfull
// execute task with gtest pre task with multiple suites with failed tests in each suite
// execute gtest task with sleep in it and abort it
// execute gtest task with failures and sleep in it and abort it