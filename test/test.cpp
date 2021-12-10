#include "blockingconcurrentqueue.h"
#include "concurrentqueue.h"
#include "process.hpp"
#include <cstddef>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
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

    moodycamel::BlockingConcurrentQueue<std::string> output;
    std::string stdout_buffer, stderr_buffer;
    std::unique_ptr<TinyProcessLib::Process> proc;

    virtual void TearDown() override {
        if (proc) {
            proc->kill();
        }
    }
    void run_cdt(const std::string& config_name, const std::string& home_dir) {
        const std::vector<std::string> args = {cwd / BINARY_NAME, test_configs_dir / config_name};
        const std::unordered_map<std::string, std::string> env = {{"HOME", test_homes_dir / home_dir}};
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
        const auto pos = msg.find(CDT_PREFIX);
        if (pos != std::string::npos) {
            msg = msg.substr(pos + CDT_PREFIX.size());
        }
        return msg;
    }
};

#define ASSERT_HELP_PROMPT_DISPLAYED() ASSERT_EQ("Type \x1B[32mh\x1B[0m to see list of all the user commands.", get_out_line())

#define ASSERT_TASK_LIST_DISPLAYED()\
    ASSERT_EQ("\x1B[32mTasks:\x1B[0m", get_out_line());\
    ASSERT_EQ("1 \"hello world\"", get_out_line())

#define ASSERT_HELP_DISPLAYED()\
    ASSERT_EQ("\x1B[32mUser commands:\x1B[0m", get_out_line());\
    ASSERT_EQ("t<ind>\t\tExecute the task with the specified index", get_out_line());\
    ASSERT_EQ("o<ind>\t\tOpen the file link with the specified index in your code editor", get_out_line());\
    ASSERT_EQ("h\t\tDisplay list of user commands", get_out_line())

TEST_F(cdt_test, start_and_view_tasks) {
    run_cdt("test-tasks.json", "no-config");
    ASSERT_HELP_PROMPT_DISPLAYED();
    ASSERT_TASK_LIST_DISPLAYED();
}

TEST_F(cdt_test, fail_to_start_due_to_user_config_not_being_json) {
    using namespace std::string_literals;
    run_cdt("test-tasks.json", "non-json-config");
    ASSERT_EQ("\x1B[31mFailed to parse "s + to_absolute_user_config_path("non-json-config") + ": [json.exception.parse_error.101] parse error at line 1, column 1: syntax error while parsing value - invalid literal; last read: 'p'", get_out_line());
}

TEST_F(cdt_test, fail_to_start_due_to_user_config_having_open_in_editor_command_in_incorrect_format) {
    run_cdt("test-tasks.json", "incorrect-open-in-editor-command");
    ASSERT_EQ(is_invalid_error_message(to_absolute_user_config_path("incorrect-open-in-editor-command")), get_out_line());
    ASSERT_EQ("'open_in_editor_command': must be a string in format: 'notepad++ {}', where {} will be replaced with a file name", get_out_line());
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_specified) {
    const std::vector<std::string> args = {cwd / BINARY_NAME};
    proc = std::make_unique<TinyProcessLib::Process>(args, "", write_to(output, stdout_buffer), write_to(output, stderr_buffer), true);
    ASSERT_EQ("\x1B[31musage: cpp-dev-tools tasks.json", get_out_line());
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_existing) {
    using namespace std::string_literals;
    run_cdt("non-existent-config.json", "no-config");
    ASSERT_EQ("\x1B[31m"s + to_absolute_tasks_config_path("non-existent-config.json") + " does not exist", get_out_line());
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_being_json) {
    using namespace std::string_literals;
    run_cdt("non-json-config.json", "no-config");
    ASSERT_EQ("\x1B[31mFailed to parse "s + to_absolute_tasks_config_path("non-json-config.json") + ": [json.exception.parse_error.101] parse error at line 1, column 1: syntax error while parsing value - invalid literal; last read: 'p'", get_out_line());
}

TEST_F(cdt_test, fail_to_start_due_to_cdt_tasks_not_being_specified_in_config) {
    run_cdt("cdt_tasks-not-specified.json", "no-config");
    ASSERT_EQ(is_invalid_error_message(to_absolute_tasks_config_path("cdt_tasks-not-specified.json")), get_out_line());
    ASSERT_EQ("'cdt_tasks': must be an array of task objects", get_out_line());
}

TEST_F(cdt_test, fail_to_start_due_to_cdt_tasks_not_being_array_of_objects) {
    run_cdt("cdt_tasks-not-array.json", "no-config");
    ASSERT_EQ(is_invalid_error_message(to_absolute_tasks_config_path("cdt_tasks-not-array.json")), get_out_line());
    ASSERT_EQ("'cdt_tasks': must be an array of task objects", get_out_line());
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_having_errors) {
    run_cdt("config-with-errors.json", "no-config");
    ASSERT_EQ(is_invalid_error_message(to_absolute_tasks_config_path("config-with-errors.json")), get_out_line());
    ASSERT_EQ("task #1: 'name': must be a string", get_out_line());
    ASSERT_EQ("task #2: 'command': must be a string", get_out_line());
    ASSERT_EQ("task #2: 'pre_tasks': must be an array of other task names", get_out_line());
    ASSERT_EQ("task #3: references task 'non-existent-task' that does not exist", get_out_line());
    ASSERT_EQ("task 'cycle-1' has a circular dependency in it's 'pre_tasks':", get_out_line());
    ASSERT_EQ("cycle-1 -> cycle-2 -> cycle-3 -> cycle-1", get_out_line());
    ASSERT_EQ("task 'cycle-2' has a circular dependency in it's 'pre_tasks':", get_out_line());
    ASSERT_EQ("cycle-2 -> cycle-3 -> cycle-1 -> cycle-2", get_out_line());
    ASSERT_EQ("task 'cycle-3' has a circular dependency in it's 'pre_tasks':", get_out_line());
    ASSERT_EQ("cycle-3 -> cycle-1 -> cycle-2 -> cycle-3", get_out_line());
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

// TODO:
// execute a single task
// execute a task with pre_tasks with pre_tasks
// fail task
// fail one of task's pre_tasks
// abort active task with SIGINT
// exit with SIGINT
// execute restart task (check that restart command persist through restart)
// highlight file links in task output and open one of them if open_in_editor_command is specified
// highlight file links in failed task output and open one of them if open_in_editor_command is specified
// highlight file links in failed pre_task output and open one of them if open_in_editor_command is specified
// display task output when attempting to open non-existent link
// display failed task output when attempting to open non-existent link
// display failed pre_task output when attempting to open non-existent link
// when attempting to open a link with open_in_editor_command not set - warn user about it
// execute last user command on enter