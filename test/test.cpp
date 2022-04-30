#include <cstddef>
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-cardinalities.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cdt.h"
#include "json.hpp"
#include "process.hpp"

struct process_output {
    std::string data;
    bool is_stdout = true;
};

struct process_exec {
    std::vector<process_output> output_lines;
    bool is_long = false;
    int exit_code = 0;
};

struct process_exit_info {
    int exit_code;
    std::function<void()> exit_cb;
};

class os_api_mock: public os_api {
public:
    MOCK_METHOD(std::istream&, in, (), (override));
    MOCK_METHOD(std::ostream&, out, (), (override));
    MOCK_METHOD(std::ostream&, err, (), (override));
    MOCK_METHOD(std::string, get_env, (const std::string&), (override));
    MOCK_METHOD(void, set_env, (const std::string&, const std::string&), (override));
    MOCK_METHOD(void, set_current_path, (const std::filesystem::path&), (override));
    MOCK_METHOD(std::filesystem::path, get_current_path, (), (override));
    MOCK_METHOD(bool, read_file, (const std::filesystem::path&, std::string&), (override));
    MOCK_METHOD(void, write_file, (const std::filesystem::path&, const std::string&), (override));
    MOCK_METHOD(bool, file_exists, (const std::filesystem::path&), (override));
    MOCK_METHOD(void, signal, (int, void(*)(int)), (override));
    MOCK_METHOD(void, raise_signal, (int), (override));
    MOCK_METHOD(int, exec, (const std::vector<const char*>&), (override));
    MOCK_METHOD(void, exec_process, (const std::string&), (override));
    void kill_process(entity e, std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>>& map) override {
        process_exit_info& info = proc_exit_info.at(e);
        info.exit_cb();
        info.exit_code = -1;
    }
    void start_process(entity e, const std::string& cmd, const std::function<void(const char*, size_t)>& stdout_cb, const std::function<void(const char*, size_t)>& stderr_cb, const std::function<void()>& exit_cb, std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>>& map) override {
        auto it = cmd_to_process_execs.find(cmd);
        if (it == cmd_to_process_execs.end()) {
            throw std::runtime_error("Unexpected command called: " + cmd);
        }
        std::deque<process_exec>& execs = it->second;
        process_exec exec = execs.front();
        if (execs.size() > 1) {
            execs.pop_front();
        }
        map[e] = std::unique_ptr<TinyProcessLib::Process>();
        proc_exit_info[e] = process_exit_info{exec.exit_code, exit_cb};
        for (process_output& line: exec.output_lines) {
            if (line.is_stdout) {
                stdout_cb(line.data.data(), line.data.size());
            } else {
                stderr_cb(line.data.data(), line.data.size());
            }
        }
        if (!exec.is_long) {
            exit_cb();
        }
    }
    int get_process_exit_code(entity e, std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>>& map) override {
        return proc_exit_info.at(e).exit_code;
    }
    void mock_read_file(const std::filesystem::path& p, const std::string& d) {
        EXPECT_CALL(*this, read_file(testing::Eq(p), testing::_)).WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(d), testing::Return(true)));
    }
    void mock_read_file(const std::filesystem::path& p) {
        EXPECT_CALL(*this, read_file(testing::Eq(p), testing::_)).WillRepeatedly(testing::Return(false));
    }

    std::unordered_map<std::string, std::deque<process_exec>> cmd_to_process_execs;
    std::unordered_map<entity, process_exit_info> proc_exit_info;
};

class cdt_test: public testing::Test {
public:
    std::string default_user_config_content =
    "{\n"
    "  // Open file links from the output in Sublime Text:\n"
    "  //\"open_in_editor_command\": \"subl {}\"\n"
    "  // Open file links from the output in VSCode:\n"
    "  //\"open_in_editor_command\": \"code {}\"\n"
    "  // Execute in a new terminal tab on MacOS:\n"
    "  // \"execute_in_new_terminal_tab_command\": \"osascript -e 'tell application \\\"Terminal\\\" to do script \\\"{}\\\"'\"\n"
    "  // Execute in a new terminal tab on Windows:\n"
    "  // \"execute_in_new_terminal_tab_command\": \"/c/Program\\ Files/Git/git-bash -c '{}'\"\n"
    "  // Debug tasks via lldb:\n"
    "  //\"debug_command\": \"lldb -- {}\"\n"
    "  // Debug tasks via gdb:\n"
    "  //\"debug_command\": \"gdb --args {}\"\n"
    "}\n";
    const char* tasks_config_filename = "tasks.json";
    std::string editor_exec = "subl";
    std::string new_terminal_tab_exec = "terminal";
    std::string debugger_exec = "lldb";
    std::string hello_world_task_cmd = "echo hello world!";
    std::string tests_task_cmd = "tests";
    std::filesystem::path home_path = std::filesystem::path("/users/my-user");
    std::filesystem::path user_config_path = home_path / ".cpp-dev-tools.json";
    std::filesystem::path tasks_config_path = std::filesystem::absolute(tasks_config_filename);
    process_exec successful_gtest_exec, failed_gtest_exec, successful_single_gtest_exec, failed_single_gtest_exec, aborted_gtest_exec;
    std::stringstream in, out, eout; // eout - expected out
    std::vector<const char*> argv = {"cpp-dev-tools", tasks_config_filename};
    std::function<void(int)> sigint_handler;
    testing::NiceMock<os_api_mock> mock;
    cdt cdt;

    void SetUp() override {
        cdt.os = &mock;
        EXPECT_CALL(mock, in()).Times(testing::AnyNumber()).WillRepeatedly(testing::ReturnRef(in));
        EXPECT_CALL(mock, out()).Times(testing::AnyNumber()).WillRepeatedly(testing::ReturnRef(out));
        EXPECT_CALL(mock, err()).Times(testing::AnyNumber()).WillRepeatedly(testing::ReturnRef(out));
        EXPECT_CALL(mock, get_env("HOME")).Times(testing::AnyNumber()).WillRepeatedly(testing::Return(home_path));
        EXPECT_CALL(mock, get_env("LAST_COMMAND")).Times(testing::AnyNumber()).WillRepeatedly(testing::Return(""));
        EXPECT_CALL(mock, get_current_path()).Times(testing::AnyNumber()).WillRepeatedly(testing::Return(tasks_config_path.parent_path()));
        EXPECT_CALL(mock, signal(testing::Eq(SIGINT), testing::_)).WillRepeatedly(testing::SaveArg<1>(&sigint_handler));
        // mock tasks config
        std::vector<nlohmann::json> tasks;
        tasks.push_back(create_task_and_process("hello world!"));
        tasks.push_back(create_task_and_process("primary task", {"pre task 1", "pre task 2"}));
        tasks.push_back(create_task_and_process("pre task 1", {"pre pre task 1", "pre pre task 2"}));
        tasks.push_back(create_task_and_process("pre task 2"));
        tasks.push_back(create_task_and_process("pre pre task 1"));
        tasks.push_back(create_task_and_process("pre pre task 2"));
        tasks.push_back(create_task("restart", "__restart"));
        tasks.push_back(create_task("run tests", "__gtest " + tests_task_cmd));
        tasks.push_back(create_task_and_process("task with gtest pre task", {"run tests"}));
        tasks.push_back(create_task("run tests with pre tasks", "__gtest " + tests_task_cmd, {"pre pre task 1", "pre pre task 2"}));
        nlohmann::json tasks_config_data;
        tasks_config_data["cdt_tasks"] = tasks;
        mock.mock_read_file(tasks_config_path, tasks_config_data.dump());
        // mock user config
        nlohmann::json user_config_data;
        user_config_data["open_in_editor_command"] = editor_exec + " {}";
        user_config_data["execute_in_new_terminal_tab_command"] = new_terminal_tab_exec + " {}";
        user_config_data["debug_command"] = debugger_exec + " {}";
        mock.mock_read_file(user_config_path, user_config_data.dump());
        EXPECT_CALL(mock, file_exists(user_config_path)).WillRepeatedly(testing::Return(true));
        // mock default test execution
        mock_process_output(successful_gtest_exec.output_lines, {
            "Running main() from /lib/gtest_main.cc",
            "[==========] Running 3 tests from 2 test suites.",
            "[----------] Global test environment set-up.",
            "[----------] 2 tests from test_suit_1",
            "[ RUN      ] test_suit_1.test1",
            "/a/b/c:10",
            "some random data",
            "/d/e/f:15:32 something",
            "line /a/b/c:11 and /b/c:32:1",
            "[       OK ] test_suit_1.test1 (0 ms)",
            "[ RUN      ] test_suit_1.test2",
            "[       OK ] test_suit_1.test2 (0 ms)",
            "[----------] 2 tests from test_suit_1 (0 ms total)",
            "",
            "[----------] 1 test from test_suit_2",
            "[ RUN      ] test_suit_2.test1",
            "[       OK ] test_suit_2.test1 (0 ms)",
            "[----------] 1 test from test_suit_2 (0 ms total)",
            "",
            "[----------] Global test environment tear-down",
            "[==========] 3 tests from 2 test suites ran. (0 ms total)",
            "[  PASSED  ] 3 tests."
        });
        aborted_gtest_exec.exit_code = 1;
        mock_process_output(aborted_gtest_exec.output_lines, {
            "Running main() from /lib/gtest_main.cc",
            "[==========] Running 2 tests from 2 test suites.",
            "[----------] Global test environment set-up.",
            "[----------] 1 test from normal_tests",
            "[ RUN      ] normal_tests.hello_world",
            "[       OK ] normal_tests.hello_world (0 ms)",
            "[----------] 1 test from normal_tests (0 ms total)",
            "",
            "[----------] 1 test from exit_tests",
            "[ RUN      ] exit_tests.exit_in_the_middle"
        });
        failed_gtest_exec.exit_code = 1;
        mock_process_output(failed_gtest_exec.output_lines, {
            "Running main() from /lib/gtest_main.cc",
            "[==========] Running 3 tests from 2 test suites.",
            "[----------] Global test environment set-up.",
            "[----------] 2 tests from failed_test_suit_1",
            "[ RUN      ] failed_test_suit_1.test1",
            "/a/b/c:10",
            "some random data",
            "/d/e/f:15:32 something",
            "line /a/b/c:11 and /b/c:32:1",
            "unknown file: Failure",
            "C++ exception with description "" thrown in the test body.",
            "[  FAILED  ] failed_test_suit_1.test1 (0 ms)",
            "[ RUN      ] failed_test_suit_1.test2",
            "[       OK ] failed_test_suit_1.test2 (0 ms)",
            "[----------] 2 tests from failed_test_suit_1 (0 ms total)",
            "",
            "[----------] 1 test from failed_test_suit_2",
            "[ RUN      ] failed_test_suit_2.test1",
            "",
            "unknown file: Failure",
            "C++ exception with description "" thrown in the test body.",
            "[  FAILED  ] failed_test_suit_2.test1 (0 ms)",
            "[----------] 1 test from failed_test_suit_2 (0 ms total)",
            "",
            "[----------] Global test environment tear-down",
            "[==========] 3 tests from 2 test suites ran. (0 ms total)",
            "[  PASSED  ] 1 test.",
            "[  FAILED  ] 2 tests, listed below:",
            "[  FAILED  ] failed_test_suit_1.test1",
            "[  FAILED  ] failed_test_suit_2.test1",
            "",
            "2 FAILED TESTS"
        });
        mock_process_output(successful_single_gtest_exec.output_lines, {
            "Running main() from /lib/gtest_main.cc",
            "Note: Google Test filter = test_suit_1.test1",
            "[==========] Running 1 test from 1 test suite.",
            "[----------] Global test environment set-up.",
            "[----------] 1 test from test_suit_1",
            "[ RUN      ] test_suit_1.test1",
            "/a/b/c:10",
            "some random data",
            "/d/e/f:15:32 something",
            "line /a/b/c:11 and /b/c:32:1",
            "[       OK ] test_suit_1.test1 (0 ms)",
            "[----------] 1 test from test_suit_1 (0 ms total)",
            "",
            "[----------] Global test environment tear-down",
            "[==========] 1 test from 1 test suite ran. (0 ms total)",
            "[  PASSED  ] 1 test."
        });
        failed_single_gtest_exec.exit_code = 1;
        mock_process_output(failed_single_gtest_exec.output_lines, {
            "Running main() from /lib/gtest_main.cc",
            "[==========] Running 2 tests from 1 test suite.",
            "[----------] Global test environment set-up.",
            "[----------] 2 tests from failed_test_suit_1",
            "[ RUN      ] failed_test_suit_1.test1",
            "/a/b/c:10",
            "some random data",
            "/d/e/f:15:32 something",
            "line /a/b/c:11 and /b/c:32:1",
            "unknown file: Failure",
            "C++ exception with description "" thrown in the test body.",
            "[  FAILED  ] failed_test_suit_1.test1 (0 ms)",
            "[ RUN      ] failed_test_suit_1.test2",
            "[       OK ] failed_test_suit_1.test2 (0 ms)",
            "[----------] 2 tests from failed_test_suit_1 (0 ms total)",
            "",
            "[----------] Global test environment tear-down",
            "[==========] 2 tests from 1 test suite ran. (0 ms total)",
            "[  PASSED  ] 1 test.",
            "[  FAILED  ] 1 test, listed below:",
            "[  FAILED  ] failed_test_suit_1.test1",
            "",
            " 1 FAILED TEST"
        });
        mock.cmd_to_process_execs[tests_task_cmd].push_back(successful_gtest_exec);
        mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='test_suit_1.test1'"].push_back(successful_single_gtest_exec);
    }
    bool init_test_cdt() {
        return init_cdt(argv.size(), argv.data(), cdt);
    }
    nlohmann::json create_task(const std::string& name, const std::string& command, std::vector<std::string> pre_tasks = {}) {
        nlohmann::json task;
        task["name"] = name;
        task["command"] = command;
        task["pre_tasks"] = std::move(pre_tasks);
        return task;
    }
    nlohmann::json create_task_and_process(const std::string& name, std::vector<std::string> pre_tasks = {}) {
        std::string cmd = "echo " + name;
        process_exec exec;
        exec.output_lines.push_back(process_output{name + '\n'});
        mock.cmd_to_process_execs[cmd].push_back(exec);
        return create_task(name, cmd, std::move(pre_tasks));
    }
    void run_cmd(const std::string& cmd, bool break_when_process_events_stop = false) {
        eout << "\x1B[32m(cdt) \x1B[0m";
        in << cmd << std::endl;
        while (true) {
            exec_cdt_systems(cdt);
            if (!break_when_process_events_stop && will_wait_for_input(cdt)) {
                break;
            }
            if (break_when_process_events_stop && cdt.exec_event_queue.size_approx() == 0 && cdt.execs_to_run_in_order.size() == 1 && cdt.processes.size() == 1) {
                break;
            }
        }
    }
    void expect_help_prompt() {
        eout << "Type \x1B[32mh\x1B[0m to see list of all the user commands." << std::endl;
    }
    void expect_list_of_tasks() {
        eout << "\x1B[32mTasks:\x1B[0m" << std::endl;
        eout << "1 \"hello world!\"" << std::endl;
        eout << "2 \"primary task\"" << std::endl;
        eout << "3 \"pre task 1\"" << std::endl;
        eout << "4 \"pre task 2\"" << std::endl;
        eout << "5 \"pre pre task 1\"" << std::endl;
        eout << "6 \"pre pre task 2\"" << std::endl;
        eout << "7 \"restart\"" << std::endl;
        eout << "8 \"run tests\"" << std::endl;
        eout << "9 \"task with gtest pre task\"" << std::endl;
        eout << "10 \"run tests with pre tasks\"" << std::endl;
    }
    void expect_help() {
        eout << "\x1B[32mUser commands:\x1B[0m" << std::endl;
        eout << "t<ind>\t\tExecute the task with the specified index" << std::endl;
        eout << "tr<ind>\t\tKeep executing the task with the specified index until it fails" << std::endl;
        eout << "d<ind>\t\tExecute the task with the specified index with a debugger attached" << std::endl;
        eout << "o<ind>\t\tOpen the file link with the specified index in your code editor" << std::endl;
        eout << "s\t\tSearch through output of the last executed task with the specified regular expression" << std::endl;
        eout << "g<ind>\t\tDisplay output of the specified google test" << std::endl;
        eout << "gs<ind>\t\tSearch through output of the specified google test with the specified regular expression" << std::endl;
        eout << "gt<ind>\t\tRe-run the google test with the specified index" << std::endl;
        eout << "gtr<ind>\tKeep re-running the google test with the specified index until it fails" << std::endl;
        eout << "gd<ind>\t\tRe-run the google test with the specified index with debugger attached" << std::endl;
        eout << "gf<ind>\t\tRun google tests of the task with the specified index with a specified --gtest_filter" << std::endl;
        eout << "h\t\tDisplay list of user commands" << std::endl;
    }
    void expect_hello_world_task(int exit_code) {
        eout << "\x1B[35mRunning \"hello world!\"\x1B[0m" << std::endl;
        eout << "hello world!" << std::endl;
        if (exit_code == 0) {
            eout << "\x1B[35m'hello world!' complete: return code: 0\x1B[0m" << std::endl;
        } else {
            eout << "\x1B[31m'hello world!' failed: return code: " << exit_code << "\x1B[0m" << std::endl;
        }
    }
    void expect_output_with_links() {
        eout << "\x1B[35m[o1] /a/b/c:10\x1B[0m" << std::endl;
        eout << "some random data" << std::endl;
        eout << "\x1B[35m[o2] /d/e/f:15:32\x1B[0m something" << std::endl;
        eout << "line \x1B[35m[o3] /a/b/c:11\x1B[0m and \x1B[35m[o4] /b/c:32:1\x1B[0m" << std::endl;
    }
    void expect_tests_not_gtest_executable(int exit_code) {
        eout << "\x1B[35mRunning \"run tests\"\x1B[0m" << std::endl;
        eout << "\x1B[2K\r\x1B[31m'tests' is not a google test executable\x1B[0m" << std::endl;
        eout << "\x1B[31m'run tests' failed: return code: " << exit_code << "\x1B[0m" << std::endl;
    }
    void expect_tests_completed(int total, int completed = std::numeric_limits<int>().max()) {
        for (int i = 0; i < total && i < completed; i++) {
            eout << "\rTests completed: " << i + 1 << " of " << total;
        }
    }
    void expect_tests_aborted(int exit_code, const char* task_name = "run tests") {
        eout << "\x1B[2K\r\x1B[31mTests have finished prematurely\x1B[0m" << std::endl;
        eout << "\x1B[31mFailed tests:\x1B[0m" << std::endl;
        eout << "1 \"exit_tests.exit_in_the_middle\" " << std::endl;
        eout << "\x1B[31mTests failed: 1 of 2 (50%) \x1B[0m" << std::endl;
        eout << "\x1B[31m\"exit_tests.exit_in_the_middle\" output:\x1B[0m" << std::endl;
        eout << "\x1B[31m'" << task_name << "' failed: return code: " << exit_code << "\x1B[0m" << std::endl;
    }
    void expect_tests_failed(bool is_execution_result = true, const char* task_name = "run tests") {
        if (is_execution_result) {
            eout << "\x1B[2K\r";
        }
        eout << "\x1B[31mFailed tests:\x1B[0m" << std::endl;
        eout << "1 \"failed_test_suit_1.test1\" (0 ms)" << std::endl;
        eout << "2 \"failed_test_suit_2.test1\" (0 ms)" << std::endl;
        eout << "\x1B[31mTests failed: 2 of 3 (66%) (0 ms total)\x1B[0m" << std::endl;
        if (is_execution_result) {
            eout << "\x1B[31m'" << task_name << "' failed: return code: 1\x1B[0m" << std::endl;
        }
    }
    void expect_tests_successfull() {
        eout << "\x1B[35mRunning \"run tests\"\x1B[0m" << std::endl;
        expect_tests_completed(3);
        eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
        eout << "\x1B[35m'run tests' complete: return code: 0\x1B[0m" << std::endl;
    }
    void expect_single_test_failed() {
        eout << "\x1B[2K\r\x1B[31mFailed tests:\x1B[0m" << std::endl;
        eout << "1 \"failed_test_suit_1.test1\" (0 ms)" << std::endl;
        eout << "\x1B[31mTests failed: 1 of 2 (50%) (0 ms total)\x1B[0m" << std::endl;
        eout << "\x1B[31m\"failed_test_suit_1.test1\" output:\x1B[0m" << std::endl;
        expect_output_with_links();
        eout << "unknown file: Failure" << std::endl;
        eout << "C++ exception with description  thrown in the test body." << std::endl;
        eout << "\x1B[31m'run tests' failed: return code: 1\x1B[0m" << std::endl;
    }
    void expect_last_completed_tests() {
        eout << "\x1B[32mLast executed tests (0 ms total):\x1B[0m" << std::endl;
        eout << "1 \"test_suit_1.test1\" (0 ms)" << std::endl;
        eout << "2 \"test_suit_1.test2\" (0 ms)" << std::endl;
        eout << "3 \"test_suit_2.test1\" (0 ms)" << std::endl;
    }
    void expect_mandatory_debug_properties_not_specified() {
        eout << "\x1B[31m'debug_command' is not specified in " << user_config_path << "\x1B[0m" << std::endl;
        eout << "\x1B[31m'execute_in_new_terminal_tab_command' is not specified in " << user_config_path << "\x1B[0m" << std::endl;
    }
    void expect_running(const std::vector<std::string>& task_names, std::optional<std::string> primary_task_name = {}) {
        for (const std::string& name: task_names) {
            eout << "\x1B[34mRunning \"" << name << "\"...\x1B[0m" << std::endl;
        }
        if (primary_task_name) {
            eout << "\x1B[35mRunning \"" << *primary_task_name << "\"\x1B[0m" << std::endl;
        }
    }
    void mock_process_output_with_links(std::vector<process_output>& output) {
        output.clear();
        mock_process_output(output, {
           "/a/b/c:10",
           "some random data",
           "/d/e/f:15:32 something",
           "line /a/b/c:11 and /b/c:32:1"
        });
    }
    void mock_process_output(std::vector<process_output>& output, const std::vector<std::string>& lines) {
        output.clear();
        output.reserve(lines.size());
        for (const std::string& line: lines) {
            output.push_back(process_output{line + "\n"});
        }
    }
    std::string get_out_line() {
        std::string line;
        std::getline(out, line);
        return line;
    }
};

#define EXPECT_OUTEQ()\
    EXPECT_EQ(eout.str(), out.str());\
    eout.str("");\
    out.str("")
#define EXPECT_OUTPUT_LINKS_TO_OPEN()\
    testing::InSequence seq;\
    EXPECT_CALL(mock, exec_process(editor_exec + " /a/b/c:10"));\
    EXPECT_CALL(mock, exec_process(editor_exec + " /d/e/f:15:32"));\
    EXPECT_CALL(mock, exec_process(editor_exec + " /a/b/c:11"));\
    EXPECT_CALL(mock, exec_process(editor_exec + " /b/c:32:1"));\
    run_cmd("o1");\
    run_cmd("o2");\
    run_cmd("o3");\
    run_cmd("o4")
#define EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS()\
    EXPECT_CALL(mock, exec_process(testing::_)).Times(0);\
    run_cmd("o0");\
    eout << "\x1B[32mLast execution output:\x1B[0m" << std::endl;\
    expect_output_with_links();\
    run_cmd("o999");\
    eout << "\x1B[32mLast execution output:\x1B[0m" << std::endl;\
    expect_output_with_links();\
    run_cmd("o");\
    eout << "\x1B[32mLast execution output:\x1B[0m" << std::endl;\
    expect_output_with_links();\
    EXPECT_OUTEQ()
#define EXPECT_CDT_STARTED()\
    EXPECT_TRUE(init_test_cdt());\
    expect_help_prompt();\
    expect_list_of_tasks();\
    EXPECT_OUTEQ()
#define EXPECT_DEBUGGER_CALL(CMD) EXPECT_CALL(mock, exec_process("terminal cd " + tasks_config_path.parent_path().string() + " && lldb " + CMD))

MATCHER_P(StrVecEq, expected, "") {
    if (arg.size() != expected.size()) {
        return false;
    }
    for (int i = 0; i < expected.size(); i++) {
        const char* e = expected[i];
        const char* a = arg[i];
        if (e != a && strcmp(e, a) != 0) {
            return false;
        }
    }
    return true;
}

TEST_F(cdt_test, start_and_view_tasks) {
    EXPECT_CDT_STARTED();
}

TEST_F(cdt_test, fail_to_start_due_to_user_config_not_being_json) {
    mock.mock_read_file(user_config_path, "not a json");
    EXPECT_FALSE(init_test_cdt());
    eout << "\x1B[31mFailed to parse " << user_config_path.string() << ": [json.exception.parse_error.101] parse error at line 1, column 2: syntax error while parsing value - invalid literal; last read: 'no'" << std::endl;
    eout << "\x1B[0m";
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, fail_to_start_due_to_user_config_having_properties_in_incorrect_format) {
    nlohmann::json user_config_data;
    user_config_data["open_in_editor_command"] = "my-editor";
    user_config_data["execute_in_new_terminal_tab_command"] = "my-terminal";
    user_config_data["debug_command"] = "my-debugger";
    mock.mock_read_file(user_config_path, user_config_data.dump());
    EXPECT_FALSE(init_test_cdt());
    eout << "\x1B[31m" << user_config_path.string() << " is invalid:" << std::endl;
    eout << "'open_in_editor_command': must be a string in format, examples of which you can find in the config" << std::endl;
    eout << "'debug_command': must be a string in format, examples of which you can find in the config" << std::endl;
    eout << "'execute_in_new_terminal_tab_command': must be a string in format, examples of which you can find in the config" << std::endl;
    eout << "\x1B[0m";
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_specified) {
    argv.pop_back();
    EXPECT_FALSE(init_test_cdt());
    eout << "\x1B[31musage: cpp-dev-tools tasks.json" << std::endl;
    eout << "\x1B[0m";
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_existing) {
    mock.mock_read_file(tasks_config_path);
    EXPECT_FALSE(init_test_cdt());
    eout << "\x1B[31m/Users/maxim/dev/cpp-dev-tools/tasks.json does not exist" << std::endl;
    eout << "\x1B[0m";
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_being_json) {
    mock.mock_read_file(tasks_config_path, "not a json");
    EXPECT_FALSE(init_test_cdt());
    eout << "\x1B[31mFailed to parse " << tasks_config_path.string() << ": [json.exception.parse_error.101] parse error at line 1, column 2: syntax error while parsing value - invalid literal; last read: 'no'" << std::endl;
    eout << "\x1B[0m";
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, fail_to_start_due_to_cdt_tasks_not_being_specified_in_config) {
    mock.mock_read_file(tasks_config_path, "{}");
    EXPECT_FALSE(init_test_cdt());
    eout << "\x1B[31m" << tasks_config_path.string() << " is invalid:" << std::endl;
    eout << "'cdt_tasks': must be an array of task objects" << std::endl;
    eout << "\x1B[0m";
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, fail_to_start_due_to_cdt_tasks_not_being_array_of_objects) {
    nlohmann::json tasks_config_data;
    tasks_config_data["cdt_tasks"] = "string";
    mock.mock_read_file(tasks_config_path, tasks_config_data.dump());
    EXPECT_FALSE(init_test_cdt());
    eout << "\x1B[31m" << tasks_config_path.string() << " is invalid:" << std::endl;
    eout << "'cdt_tasks': must be an array of task objects" << std::endl;
    eout << "\x1B[0m";
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_having_errors) {
    nlohmann::json task_without_name;
    task_without_name["command"] = "command";
    nlohmann::json task_without_command_and_non_array_pre_tasks;
    task_without_command_and_non_array_pre_tasks["name"] = "name";
    task_without_command_and_non_array_pre_tasks["pre_tasks"] = true;
    nlohmann::json tasks_config_data;
    tasks_config_data["cdt_tasks"] = std::vector<nlohmann::json>{
        task_without_name,
        task_without_command_and_non_array_pre_tasks,
        create_task("name 2", "command", {"non-existent-task"}),
        create_task("cycle-1", "command", {"cycle-2"}),
        create_task("cycle-2", "command", {"cycle-3"}),
        create_task("cycle-3", "command", {"cycle-1"})
    };
    mock.mock_read_file(tasks_config_path, tasks_config_data.dump());
    EXPECT_FALSE(init_test_cdt());
    eout << "\x1B[31m" << tasks_config_path.string() << " is invalid:" << std::endl;
    eout << "task #1: 'name': must be a string" << std::endl;
    eout << "task #2: 'command': must be a string" << std::endl;
    eout << "task #2: 'pre_tasks': must be an array of other task names" << std::endl;
    eout << "task #3: references task 'non-existent-task' that does not exist" << std::endl;
    eout << "task 'cycle-1' has a circular dependency in it's 'pre_tasks':" << std::endl;
    eout << "cycle-1 -> cycle-2 -> cycle-3 -> cycle-1" << std::endl;
    eout << "task 'cycle-2' has a circular dependency in it's 'pre_tasks':" << std::endl;
    eout << "cycle-2 -> cycle-3 -> cycle-1 -> cycle-2" << std::endl;
    eout << "task 'cycle-3' has a circular dependency in it's 'pre_tasks':" << std::endl;
    eout << "cycle-3 -> cycle-1 -> cycle-2 -> cycle-3" << std::endl;
    eout << "\x1B[0m";
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_display_help) {
    EXPECT_CDT_STARTED();
    // Display help on unknown command
    run_cmd("zz");
    expect_help();
    // Display help on explicit command
    run_cmd("h");
    expect_help();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_display_list_of_tasks_on_task_command) {
    EXPECT_CDT_STARTED();
    // Display tasks list on no index
    run_cmd("t");
    expect_list_of_tasks();
    // Display tasks list on non-existent task specified
    run_cmd("t0");
    expect_list_of_tasks();
    run_cmd("t999");
    expect_list_of_tasks();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_single_task) {
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_hello_world_task(0);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_task_that_prints_to_stdout_and_stderr) {
    process_exec& exec = mock.cmd_to_process_execs[hello_world_task_cmd].front();
    exec.output_lines = {
        process_output{"stdo"},
        process_output{"stde", false},
        process_output{"ut\n"},
        process_output{"rr\n", false}
    };
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_running({}, "hello world!");
    eout << "stdout" << std::endl;
    eout << "stderr" << std::endl;
    eout << "\x1B[35m'hello world!' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_task_with_pre_tasks_with_pre_tasks) {
    EXPECT_CDT_STARTED();
    run_cmd("t2");
    expect_running({"pre pre task 1", "pre pre task 2", "pre task 1", "pre task 2"}, "primary task");
    eout << "primary task" << std::endl;
    eout << "\x1B[35m'primary task' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_fail_primary_task) {
    process_exec& exec = mock.cmd_to_process_execs[hello_world_task_cmd].front();
    exec.exit_code = 1;
    exec.output_lines = {
        process_output{"starting...\n"},
        process_output{"error!!!\n", false}
    };
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_running({}, "hello world!");
    eout << "starting..." << std::endl;
    eout << "error!!!" << std::endl;
    eout << "\x1B[31m'hello world!' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_fail_one_of_pre_tasks) {
    process_exec& exec = mock.cmd_to_process_execs["echo pre pre task 2"].front();
    exec.exit_code = 1;
    exec.output_lines = {process_output{"error!!!\n", false}};
    EXPECT_CDT_STARTED();
    run_cmd("t2");
    expect_running({"pre pre task 1", "pre pre task 2"});
    eout << "error!!!" << std::endl;
    eout << "\x1B[31m'pre pre task 2' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_task_and_abort_it) {
    mock.cmd_to_process_execs[hello_world_task_cmd][0].is_long = true;
    EXPECT_CDT_STARTED();
    run_cmd("t1", true);
    sigint_handler(SIGINT);
    exec_cdt_systems(cdt);
    expect_hello_world_task(-1);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_exit) {
    EXPECT_CDT_STARTED();
    testing::InSequence seq;
    EXPECT_CALL(mock, signal(SIGINT, SIG_DFL));
    EXPECT_CALL(mock, raise_signal(SIGINT));
    sigint_handler(SIGINT);
}

TEST_F(cdt_test, start_and_change_cwd_to_tasks_configs_directory) {
    EXPECT_CALL(mock, set_current_path(tasks_config_path.parent_path()));
    EXPECT_TRUE(init_test_cdt());
}

TEST_F(cdt_test, start_execute_single_task_and_repeate_the_last_command_on_enter) {
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_hello_world_task(0);
    run_cmd("");
    expect_hello_world_task(0);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_restart_task) {
    std::vector<const char*> expected_argv = {argv[0], tasks_config_path.c_str(), nullptr};
    EXPECT_CALL(mock, exec(StrVecEq(expected_argv))).WillRepeatedly(testing::Return(0));
    EXPECT_CALL(mock, set_env("LAST_COMMAND", "t7"));
    EXPECT_CDT_STARTED();
    run_cmd("t7");
    eout << "\x1B[35mRestarting program...\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_fail_to_execute_restart_task) {
    std::vector<const char*> expected_argv = {argv[0], tasks_config_path.c_str(), nullptr};
    EXPECT_CALL(mock, exec(StrVecEq(expected_argv))).WillRepeatedly(testing::Return(ENOEXEC));
    EXPECT_CDT_STARTED();
    run_cmd("t7");
    eout << "\x1B[35mRestarting program...\x1B[0m" << std::endl;
    eout << "\x1B[31mFailed to restart: Exec format error\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_task_and_open_links_from_output) {
    mock_process_output_with_links(mock.cmd_to_process_execs[hello_world_task_cmd].front().output_lines);
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_running({}, "hello world!");
    expect_output_with_links();
    eout << "\x1B[35m'hello world!' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_fail_to_execute_task_with_links_and_open_links_from_output) {
    process_exec& exec = mock.cmd_to_process_execs[hello_world_task_cmd].front();
    exec.exit_code = 1;
    mock_process_output_with_links(exec.output_lines);
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_running({}, "hello world!");
    expect_output_with_links();
    eout << "\x1B[31m'hello world!' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_fail_to_execute_pre_task_of_task_and_open_links_from_output) {
    process_exec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    exec.exit_code = 1;
    mock_process_output_with_links(exec.output_lines);
    EXPECT_CDT_STARTED();
    run_cmd("t3");
    expect_running({"pre pre task 1"});
    expect_output_with_links();
    eout << "\x1B[31m'pre pre task 1' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_execute_task_with_links_in_output_attempt_to_open_non_existent_link_and_view_task_output) {
    mock_process_output_with_links(mock.cmd_to_process_execs[hello_world_task_cmd].front().output_lines);
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_running({}, "hello world!");
    expect_output_with_links();
    eout << "\x1B[35m'hello world!' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_fail_to_execute_task_with_links_attempt_to_open_non_existent_link_and_view_task_output) {
    process_exec& exec = mock.cmd_to_process_execs[hello_world_task_cmd].front();
    mock_process_output_with_links(exec.output_lines);
    exec.exit_code = 1;
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_running({}, "hello world!");
    expect_output_with_links();
    eout << "\x1B[31m'hello world!' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_fail_to_execute_pre_task_of_task_attempt_to_open_non_existent_link_and_view_task_output) {
    process_exec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    mock_process_output_with_links(exec.output_lines);
    exec.exit_code = 1;
    EXPECT_CDT_STARTED();
    run_cmd("t3");
    expect_running({"pre pre task 1"});
    expect_output_with_links();
    eout << "\x1B[31m'pre pre task 1' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_attempt_to_open_non_existent_link_and_view_task_output) {
    EXPECT_CDT_STARTED();
    run_cmd("o1");
    eout << "\x1B[32mNo file links in the output\x1B[0m" << std::endl;
    run_cmd("o");
    eout << "\x1B[32mNo file links in the output\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_open_link_while_open_in_editor_command_is_not_specified_and_see_error) {
    mock.mock_read_file(user_config_path);
    EXPECT_CDT_STARTED();
    run_cmd("o");
    eout << "\x1B[31m'open_in_editor_command' is not specified in " << user_config_path << "\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_no_tests) {
    mock_process_output(mock.cmd_to_process_execs[tests_task_cmd].front().output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "[==========] Running 0 tests from 0 test suites.",
        "[==========] 0 tests from 0 test suites ran. (0 ms total)",
        "[  PASSED  ] 0 tests."
    });
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_running({}, "run tests");
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 0 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_execute_gtest_task_with_non_existent_binary_and_fail) {
    process_exec& exec = mock.cmd_to_process_execs[tests_task_cmd].front();
    exec.exit_code = 127;
    exec.output_lines = {process_output{tests_task_cmd + " does not exist\n", false}};
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_tests_not_gtest_executable(exec.exit_code);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_non_gtest_binary) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = mock.cmd_to_process_execs[hello_world_task_cmd].front();
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_tests_not_gtest_executable(0);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_non_gtest_binary_that_does_not_finish_and_abort_it_manually) {
    process_exec exec = mock.cmd_to_process_execs[hello_world_task_cmd].front();
    exec.is_long = true;
    mock.cmd_to_process_execs[tests_task_cmd].front() = exec;
    EXPECT_CDT_STARTED();
    run_cmd("t8", true);
    sigint_handler(SIGINT);
    exec_cdt_systems(cdt);
    expect_running({}, "run tests");
    eout << "\x1B[2K\r\x1B[31m'tests' is not a google test executable\x1B[0m" << std::endl;
    eout << "\x1B[31m'run tests' failed: return code: -1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_gtest_task_that_exits_with_0_exit_code_in_the_middle) {
    aborted_gtest_exec.exit_code = 0;
    mock.cmd_to_process_execs[tests_task_cmd].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_running({}, "run tests");
    expect_tests_completed(2, 1);
    expect_tests_aborted(0);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_gtest_task_that_exits_with_1_exit_code_in_the_middle) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_running({}, "run tests");
    expect_tests_completed(2, 1);
    expect_tests_aborted(1);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_execute_task_with_gtest_pre_task_that_exits_with_0_exit_code_in_the_middle_and_fail) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t9");
    expect_running({"run tests"});
    expect_tests_aborted(1);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_multiple_suites) {
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_tests_successfull();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_multiple_suites_each_having_failed_tests) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_running({}, "run tests");
    expect_tests_completed(3);
    expect_tests_failed();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_task_with_gtest_pre_task_with_multiple_suites) {
    EXPECT_CDT_STARTED();
    run_cmd("t9");
    expect_running({"run tests"});
    eout << "\x1B[2K\r";
    expect_running({}, "task with gtest pre task");
    eout << "task with gtest pre task" << std::endl;
    eout << "\x1B[35m'task with gtest pre task' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_task_with_gtest_pre_task_with_multiple_suites_each_having_failed_tests) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t9");
    expect_running({"run tests"});
    expect_tests_failed();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_long_test_and_abort_it) {
    aborted_gtest_exec.is_long = true;
    mock.cmd_to_process_execs[tests_task_cmd].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t8", true);
    expect_running({}, "run tests");
    expect_tests_completed(2, 1);
    sigint_handler(SIGINT);
    exec_cdt_systems(cdt);
    expect_tests_aborted(-1);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_failed_tests_and_long_test_and_abort_it) {
    process_exec& exec = mock.cmd_to_process_execs[tests_task_cmd].front();
    exec.exit_code = 1;
    exec.is_long = true;
    mock_process_output(exec.output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "[==========] Running 2 tests from 2 test suites.",
        "[----------] Global test environment set-up.",
        "[----------] 1 test from failed_test_suit_1",
        "[ RUN      ] failed_test_suit_1.test1",
        "/a/b/c:10",
        "some random data",
        "/d/e/f:15:32 something",
        "line /a/b/c:11 and /b/c:32:1",
        "unknown file: Failure",
        "C++ exception with description "" thrown in the test body.",
        "[  FAILED  ] failed_test_suit_1.test1 (0 ms)",
        "[----------] 1 test from failed_test_suit_1 (0 ms total)",
        "",
        "[----------] 1 test from long_tests",
        "[ RUN      ] long_tests.test1"
    });
    EXPECT_CDT_STARTED();
    run_cmd("t8", true);
    expect_running({}, "run tests");
    expect_tests_completed(2, 1);
    sigint_handler(SIGINT);
    exec_cdt_systems(cdt);
    eout << "\x1B[2K\r\x1B[31mTests have finished prematurely\x1B[0m" << std::endl;
    eout << "\x1B[31mFailed tests:\x1B[0m" << std::endl;
    eout << "1 \"failed_test_suit_1.test1\" (0 ms)" << std::endl;
    eout << "2 \"long_tests.test1\" " << std::endl;
    eout << "\x1B[31mTests failed: 2 of 2 (100%) \x1B[0m" << std::endl;
    eout << "\x1B[31m'run tests' failed: return code: -1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_repeatedly_execute_task_until_it_fails) {
    mock.cmd_to_process_execs[tests_task_cmd].push_back(successful_gtest_exec);
    mock.cmd_to_process_execs[tests_task_cmd].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    run_cmd("tr8");
    expect_tests_successfull();
    expect_tests_successfull();
    expect_running({}, "run tests");
    expect_tests_completed(3);
    expect_tests_failed();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_repeatedly_execute_task_until_one_of_its_pre_tasks_fails) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("tr9");
    expect_running({"run tests"});
    expect_tests_failed();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_repeatedly_execute_long_task_and_abort_it) {
    mock.cmd_to_process_execs[hello_world_task_cmd].front().is_long = true;
    EXPECT_CDT_STARTED();
    run_cmd("tr1", true);
    sigint_handler(SIGINT);
    expect_hello_world_task(-1);
    exec_cdt_systems(cdt);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_view_gtest_tests_but_see_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    run_cmd("g");
    eout << "\x1B[32mNo google tests have been executed yet.\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_try_to_view_gtest_test_output_with_index_out_of_range_and_see_all_tests_list) {
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_tests_successfull();
    run_cmd("g0");
    expect_last_completed_tests();
    run_cmd("g99");
    expect_last_completed_tests();
    run_cmd("g");
    expect_last_completed_tests();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_view_gtest_task_output_with_file_links_highlighted_in_the_output_and_open_links) {
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_tests_successfull();
    run_cmd("g1");
    eout << "\x1B[32m\"test_suit_1.test1\" output:\x1B[0m" << std::endl;
    expect_output_with_links();
    EXPECT_OUTEQ();
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_execute_gtest_task_fail_try_to_view_gtest_test_output_with_index_out_of_range_and_see_failed_tests_list) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_running({}, "run tests");
    expect_tests_completed(3);
    expect_tests_failed();
    run_cmd("g0");
    expect_tests_failed(false);
    run_cmd("g99");
    expect_tests_failed(false);
    run_cmd("g");
    expect_tests_failed(false);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_fail_view_gtest_test_output_with_file_links_highlighted_in_the_output_and_open_links) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_running({}, "run tests");
    expect_tests_completed(3);
    expect_tests_failed();
    run_cmd("g1");
    eout << "\x1B[31m\"failed_test_suit_1.test1\" output:\x1B[0m" << std::endl;
    expect_output_with_links();
    eout << "unknown file: Failure" << std::endl;
    eout << "C++ exception with description  thrown in the test body." << std::endl;
    EXPECT_OUTEQ();
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_execute_gtest_task_fail_one_of_the_tests_and_view_automatically_displayed_test_output) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_single_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_running({}, "run tests");
    expect_tests_completed(2);
    expect_single_test_failed();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_task_with_gtest_pre_task_fail_one_of_the_tests_and_view_automatically_displayed_test_output) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_single_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t9");
    expect_running({"run tests"});
    expect_single_test_failed();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_rerun_gtest_when_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    run_cmd("gt");
    eout << "\x1B[32mNo google tests have been executed yet.\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_attempt_to_rerun_test_that_does_not_exist_and_view_list_of_failed_tests) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    expect_tests_failed(true, "run tests with pre tasks");
    run_cmd("gt");
    expect_tests_failed(false);
    run_cmd("gt0");
    expect_tests_failed(false);
    run_cmd("gt99");
    expect_tests_failed(false);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_and_rerun_failed_test) {
    process_exec first_test_rerun;
    first_test_rerun.exit_code = 1;
    mock_process_output(first_test_rerun.output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "Note: Google Test filter = failed_test_suit_1.test1",
        "[==========] Running 1 test from 1 test suite.",
        "[----------] Global test environment set-up.",
        "[----------] 1 test from failed_test_suit_1",
        "[ RUN      ] failed_test_suit_1.test1",
        "/a/b/c:10",
        "some random data",
        "/d/e/f:15:32 something",
        "line /a/b/c:11 and /b/c:32:1",
        "unknown file: Failure",
        "C++ exception with description "" thrown in the test body.",
        "[  FAILED  ] failed_test_suit_1.test1 (0 ms)",
        "[----------] 1 test from failed_test_suit_1 (0 ms total)",
        "",
        "[----------] Global test environment tear-down",
        "[==========] 1 test from 1 test suite ran. (0 ms total)",
        "[  PASSED  ] 0 tests.",
        "[  FAILED  ] 1 test, listed below:",
        "[  FAILED  ] failed_test_suit_1.test1",
        "",
        " 1 FAILED TEST"
    });
    process_exec second_test_rerun;
    second_test_rerun.exit_code = 1;
    mock_process_output(first_test_rerun.output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "Note: Google Test filter = failed_test_suit_2.test1",
        "[==========] Running 1 test from 1 test suite.",
        "[----------] Global test environment set-up.",
        "[----------] 1 test from failed_test_suit_2",
        "[ RUN      ] failed_test_suit_2.test1",
        "unknown file: Failure",
        "C++ exception with description "" thrown in the test body.",
        "[  FAILED  ] failed_test_suit_2.test1 (0 ms)",
        "[----------] 1 test from failed_test_suit_2 (0 ms total)",
        "",
        "[----------] Global test environment tear-down",
        "[==========] 1 test from 1 test suite ran. (0 ms total)",
        "[  PASSED  ] 0 tests.",
        "[  FAILED  ] 1 test, listed below:",
        "[  FAILED  ] failed_test_suit_2.test1",
        "",
        " 1 FAILED TEST"
    });
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='failed_test_suit_1.test1'"].push_back(first_test_rerun);
    mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='failed_test_suit_2.test1'"].push_back(second_test_rerun);
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    expect_tests_failed(true, "run tests with pre tasks");
    run_cmd("gt1");
    expect_running({"pre pre task 1", "pre pre task 2"}, "failed_test_suit_1.test1");
    eout << "unknown file: Failure" << std::endl;
    eout << "C++ exception with description  thrown in the test body." << std::endl;
    eout << "\x1B[31m'failed_test_suit_1.test1' failed: return code: 1\x1B[0m" << std::endl;
    run_cmd("gt2");
    expect_running({"pre pre task 1", "pre pre task 2"}, "failed_test_suit_2.test1");
    eout << "\x1B[31m'failed_test_suit_2.test1' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_rerun_test_that_does_not_exist_and_view_list_of_all_tests) {
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gt");
    expect_last_completed_tests();
    run_cmd("gt0");
    expect_last_completed_tests();
    run_cmd("gt99");
    expect_last_completed_tests();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_and_rerun_one_of_test) {
    process_exec second_test_rerun;
    mock_process_output(second_test_rerun.output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "Note: Google Test filter = test_suit_1.test2",
        "[==========] Running 1 test from 1 test suite.",
        "[----------] Global test environment set-up.",
        "[----------] 1 test from test_suit_1",
        "[ RUN      ] test_suit_1.test2",
        "[       OK ] test_suit_1.test2 (0 ms)",
        "[----------] 1 test from test_suit_1 (0 ms total)",
        "",
        "[----------] Global test environment tear-down",
        "[==========] 1 test from 1 test suite ran. (0 ms total)",
        "[  PASSED  ] 1 test."
    });
    mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='test_suit_1.test2'"].push_back(second_test_rerun);
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gt1");
    expect_running({"pre pre task 1", "pre pre task 2"}, "test_suit_1.test1");
    expect_output_with_links();
    eout << "\x1B[35m'test_suit_1.test1' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gt2");
    expect_running({"pre pre task 1", "pre pre task 2"}, "test_suit_1.test2");
    eout << "\x1B[35m'test_suit_1.test2' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();    
}

TEST_F(cdt_test, start_repeatedly_execute_gtest_task_with_pre_tasks_until_it_fails_and_repeatedly_rerun_failed_test_until_it_fails) {
    process_exec successful_rerun;
    mock_process_output(successful_rerun.output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "Note: Google Test filter = failed_test_suit_1.test1",
        "[==========] Running 1 test from 1 test suite.",
        "[----------] Global test environment set-up.",
        "[----------] 1 test from failed_test_suit_1",
        "[ RUN      ] failed_test_suit_1.test1",
        "/a/b/c:10",
        "some random data",
        "/d/e/f:15:32 something",
        "line /a/b/c:11 and /b/c:32:1",
        "[       OK ] failed_test_suit_1.test1 (0 ms)",
        "[----------] 1 test from failed_test_suit_1 (0 ms total)",
        "",
        "[----------] Global test environment tear-down",
        "[==========] 1 test from 1 test suite ran. (0 ms total)",
        "[  PASSED  ] 1 test."
    });
    process_exec failed_rerun;
    failed_rerun.exit_code = 1;
    mock_process_output(failed_rerun.output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "Note: Google Test filter = failed_test_suit_1.test1",
        "[==========] Running 1 test from 1 test suite.",
        "[----------] Global test environment set-up.",
        "[----------] 1 test from failed_test_suit_1",
        "[ RUN      ] failed_test_suit_1.test1",
        "/a/b/c:10",
        "some random data",
        "/d/e/f:15:32 something",
        "line /a/b/c:11 and /b/c:32:1",
        "unknown file: Failure",
        "C++ exception with description "" thrown in the test body.",
        "[  FAILED  ] failed_test_suit_1.test1 (0 ms)",
        "[----------] 1 test from failed_test_suit_1 (0 ms total)",
        "",
        "[----------] Global test environment tear-down",
        "[==========] 1 test from 1 test suite ran. (0 ms total)",
        "[  PASSED  ] 0 tests.",
        "[  FAILED  ] 1 test, listed below:",
        "[  FAILED  ] failed_test_suit_1.test1",
        "",
        " 1 FAILED TEST"
    });
    mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='failed_test_suit_1.test1'"].push_back(successful_rerun);
    mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='failed_test_suit_1.test1'"].push_back(successful_rerun);
    mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='failed_test_suit_1.test1'"].push_back(failed_rerun);
    mock.cmd_to_process_execs[tests_task_cmd].push_back(successful_gtest_exec);
    mock.cmd_to_process_execs[tests_task_cmd].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    run_cmd("tr10");
    // First execution should succeed
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    // Second execution should succeed
    expect_running({}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    // Third execution should fail
    expect_running({}, "run tests with pre tasks");
    expect_tests_completed(3);
    expect_tests_failed(true, "run tests with pre tasks");
    run_cmd("gtr1");
    // First re-run should succeed
    expect_running({"pre pre task 1", "pre pre task 2"}, "failed_test_suit_1.test1");
    expect_output_with_links();
    eout << "\x1B[35m'failed_test_suit_1.test1' complete: return code: 0\x1B[0m" << std::endl;
    // Second re-run should succeed
    expect_running({}, "failed_test_suit_1.test1");
    expect_output_with_links();
    eout << "\x1B[35m'failed_test_suit_1.test1' complete: return code: 0\x1B[0m" << std::endl;
    // Third re-run should fail
    expect_running({}, "failed_test_suit_1.test1");
    expect_output_with_links();
    eout << "unknown file: Failure" << std::endl;
    eout << "C++ exception with description  thrown in the test body." << std::endl;
    eout << "\x1B[31m'failed_test_suit_1.test1' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_repeatedly_rerun_one_of_the_tests_and_fail_due_to_failed_pre_task) {
    process_exec failed_pre_task = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    failed_pre_task.exit_code = 1;
    mock.cmd_to_process_execs["echo pre pre task 1"].push_back(failed_pre_task);
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gt1");
    expect_running({"pre pre task 1"});
    eout << "pre pre task 1" << std::endl;
    eout << "\x1B[31m'pre pre task 1' failed: return code: 1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_long_gtest_task_with_pre_tasks_abort_it_repeatedly_rerun_failed_test_and_abort_it_again) {
    process_exec rerun;
    rerun.exit_code = 1;
    rerun.is_long = true;
    mock_process_output(rerun.output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "Note: Google Test filter = exit_tests.exit_in_the_middle",
        "[==========] Running 1 test from 1 test suite.",
        "[----------] Global test environment set-up.",
        "[----------] 1 test from exit_tests",
        "[ RUN      ] exit_tests.exit_in_the_middle"
    });
    aborted_gtest_exec.is_long = true;
    mock.cmd_to_process_execs[tests_task_cmd].front() = aborted_gtest_exec;
    mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='exit_tests.exit_in_the_middle'"].push_back(rerun);
    EXPECT_CDT_STARTED();
    run_cmd("t10", true);
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(2, 1);
    sigint_handler(SIGINT);
    exec_cdt_systems(cdt);
    expect_tests_aborted(-1, "run tests with pre tasks");
    run_cmd("gtr1", true);
    expect_running({"pre pre task 1", "pre pre task 2"}, "exit_tests.exit_in_the_middle");
    sigint_handler(SIGINT);
    exec_cdt_systems(cdt);
    eout << "\x1B[31m'exit_tests.exit_in_the_middle' failed: return code: -1\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_repeatedly_rerun_gtest_when_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    run_cmd("gtr");
    eout << "\x1B[32mNo google tests have been executed yet.\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_repeatedly_rerun_test_that_does_not_exist_and_view_list_of_all_tests) {
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gtr");
    expect_last_completed_tests();
    run_cmd("gtr0");
    expect_last_completed_tests();
    run_cmd("gtr99");
    expect_last_completed_tests();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_create_example_user_config) {
    EXPECT_CALL(mock, file_exists(user_config_path)).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(mock, write_file(user_config_path, default_user_config_content));
    EXPECT_CDT_STARTED();
}

TEST_F(cdt_test, start_and_not_override_existing_user_config) {
    EXPECT_CALL(mock, write_file(testing::Eq(user_config_path), testing::_)).Times(0);
    EXPECT_CDT_STARTED();
}

TEST_F(cdt_test, start_attempt_to_execute_google_tests_with_filter_targeting_task_that_does_not_exist_and_view_list_of_task) {
    EXPECT_CDT_STARTED();
    run_cmd("gf");
    expect_list_of_tasks();
    run_cmd("gf0");
    expect_list_of_tasks();
    run_cmd("gf99");
    expect_list_of_tasks();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_gtest_filter) {
    process_exec filtered_tests;
    mock_process_output(filtered_tests.output_lines, {
        "Running main() from /lib/gtest_main.cc",
        "[==========] Running 2 tests from 1 test suite.",
        "[----------] Global test environment set-up.",
        "[----------] 2 tests from test_suit_1",
        "[ RUN      ] test_suit_1.test1",
        "/a/b/c:10",
        "some random data",
        "/d/e/f:15:32 something",
        "line /a/b/c:11 and /b/c:32:1",
        "[       OK ] test_suit_1.test1 (0 ms)",
        "[ RUN      ] test_suit_1.test2",
        "[       OK ] test_suit_1.test2 (0 ms)",
        "[----------] 2 tests from test_suit_1 (0 ms total)",
        "",
        "[----------] Global test environment tear-down",
        "[==========] 2 tests from 1 test suite ran. (0 ms total)",
        "[  PASSED  ] 2 tests."
    });
    mock.cmd_to_process_execs[tests_task_cmd + " --gtest_filter='test_suit_1.*'"].push_back(filtered_tests);
    EXPECT_CDT_STARTED();
    run_cmd("gf8\ntest_suit_1.*");
    eout << "\x1B[32m--gtest_filter=\x1B[0m";
    eout << "\x1B[35mRunning \"test_suit_1.*\"\x1B[0m" << std::endl;
    expect_tests_completed(2);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 2 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'test_suit_1.*' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_search_execution_output_but_fail_due_to_no_task_being_executed_before_it) {
    EXPECT_CDT_STARTED();
    run_cmd("s");
    eout << "\x1B[32mNo task has been executed yet\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_task_attempt_to_search_its_output_with_invalid_regex_and_fail) {
    mock_process_output_with_links(mock.cmd_to_process_execs[hello_world_task_cmd].front().output_lines);
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_running({}, "hello world!");
    expect_output_with_links();
    eout << "\x1B[35m'hello world!' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("s\n[");
    eout << "\x1B[32mRegular expression: \x1B[0m";
    eout << "\x1B[31mInvalid regular expression '[': The expression contained mismatched [ and ].\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_task_search_its_output_and_find_no_results) {
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_hello_world_task(0);
    run_cmd("s\nnon\\-existent");
    eout << "\x1B[32mRegular expression: \x1B[0m";
    eout << "\x1B[32mNo matches found\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_task_search_its_output_and_find_results) {
    mock_process_output_with_links(mock.cmd_to_process_execs[hello_world_task_cmd].front().output_lines);
    EXPECT_CDT_STARTED();
    run_cmd("t1");
    expect_running({}, "hello world!");
    expect_output_with_links();
    eout << "\x1B[35m'hello world!' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("s\n(some|data)");
    eout << "\x1B[32mRegular expression: \x1B[0m";
    eout << "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m" << std::endl;
    eout << "\x1B[35m3:\x1B[0m\x1B[35m[o2] /d/e/f:15:32\x1B[0m \x1B[32msome\x1B[0mthing" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_search_gtest_output_when_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    run_cmd("gs");
    eout << "\x1B[32mNo google tests have been executed yet.\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_attempt_to_search_output_of_test_that_does_not_exist_and_view_list_of_failed_tests) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    expect_tests_failed(true, "run tests with pre tasks");
    run_cmd("gs");
    expect_tests_failed(false);
    run_cmd("gs0");
    expect_tests_failed(false);
    run_cmd("gs99");
    expect_tests_failed(false);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_and_search_output_of_the_test) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    expect_tests_failed(true, "run tests with pre tasks");
    run_cmd("gs1\n(C\\+\\+|with description|Failure)");
    eout << "\x1B[32mRegular expression: \x1B[0m";
    eout << "\x1B[35m5:\x1B[0munknown file: \x1B[32mFailure\x1B[0m" << std::endl;
    eout << "\x1B[35m6:\x1B[0m\x1B[32mC++\x1B[0m exception \x1B[32mwith description\x1B[0m  thrown in the test body." << std::endl;
    run_cmd("gs2\n(C\\+\\+|with description|Failure)");
    eout << "\x1B[32mRegular expression: \x1B[0m";
    eout << "\x1B[35m2:\x1B[0munknown file: \x1B[32mFailure\x1B[0m" << std::endl;
    eout << "\x1B[35m3:\x1B[0m\x1B[32mC++\x1B[0m exception \x1B[32mwith description\x1B[0m  thrown in the test body." << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_search_output_of_test_that_does_not_exist_and_view_list_of_all_tests) {
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gs");
    expect_last_completed_tests();
    run_cmd("gs0");
    expect_last_completed_tests();
    run_cmd("gs99");
    expect_last_completed_tests();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_and_search_output_of_one_of_the_tests) {
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gs1\n(some|data)");
    eout << "\x1B[32mRegular expression: \x1B[0m";
    eout << "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m" << std::endl;
    eout << "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing" << std::endl;
    run_cmd("gs2\n(some|data)");
    eout << "\x1B[32mRegular expression: \x1B[0m";
    eout << "\x1B[32mNo matches found\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_debug_task_while_mandatory_properties_are_not_specified_in_user_config) {
    mock.mock_read_file(user_config_path);
    EXPECT_CDT_STARTED();
    run_cmd("d");
    expect_mandatory_debug_properties_not_specified();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_debug_task_that_does_not_exist_and_view_list_of_all_tasks) {
    EXPECT_CDT_STARTED();
    run_cmd("d");
    expect_list_of_tasks();
    run_cmd("d0");
    expect_list_of_tasks();
    run_cmd("d99");
    expect_list_of_tasks();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_debug_primary_task_with_pre_tasks) {
    EXPECT_DEBUGGER_CALL("echo primary task");
    EXPECT_CDT_STARTED();
    run_cmd("d2");
    expect_running({"pre pre task 1", "pre pre task 2", "pre task 1", "pre task 2"});
    eout << "\x1B[35mStarting debugger for \"primary task\"...\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_debug_gtest_primary_task_with_pre_tasks) {
    EXPECT_DEBUGGER_CALL("tests");
    EXPECT_CDT_STARTED();
    run_cmd("d10");
    expect_running({"pre pre task 1", "pre pre task 2"});
    eout << "\x1B[35mStarting debugger for \"run tests with pre tasks\"...\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_rerun_gtest_with_debugger_while_mandatory_properties_are_not_specified_in_user_config) {
    mock.mock_read_file(user_config_path);
    EXPECT_CDT_STARTED();
    run_cmd("gd");
    expect_mandatory_debug_properties_not_specified();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_attempt_to_rerun_gtest_with_debugger_when_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    run_cmd("gd");
    eout << "\x1B[32mNo google tests have been executed yet.\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_attempt_to_rerun_test_that_does_not_exist_with_debugger_and_view_list_of_failed_tests) {
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    expect_tests_failed(true, "run tests with pre tasks");
    run_cmd("gd");
    expect_tests_failed(false);
    run_cmd("gd0");
    expect_tests_failed(false);
    run_cmd("gd99");
    expect_tests_failed(false);
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_and_rerun_failed_test_with_debugger) {
    testing::InSequence seq;
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='failed_test_suit_1.test1'");
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='failed_test_suit_2.test1'");
    mock.cmd_to_process_execs[tests_task_cmd].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    expect_tests_failed(true, "run tests with pre tasks");
    run_cmd("gd1");
    expect_running({"pre pre task 1", "pre pre task 2"});
    eout << "\x1B[35mStarting debugger for \"failed_test_suit_1.test1\"...\x1B[0m" << std::endl;
    run_cmd("gd2");
    expect_running({"pre pre task 1", "pre pre task 2"});
    eout << "\x1B[35mStarting debugger for \"failed_test_suit_2.test1\"...\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_rerun_test_that_does_not_exist_with_debugger_and_view_list_of_all_tests) {
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gd");
    expect_last_completed_tests();
    run_cmd("gd0");
    expect_last_completed_tests();
    run_cmd("gd99");
    expect_last_completed_tests();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_and_rerun_one_of_tests_with_debugger) {
    testing::InSequence seq;
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='test_suit_1.test1'");
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='test_suit_1.test2'");
    EXPECT_CDT_STARTED();
    run_cmd("t10");
    expect_running({"pre pre task 1", "pre pre task 2"}, "run tests with pre tasks");
    expect_tests_completed(3);
    eout << "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m" << std::endl;
    eout << "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m" << std::endl;
    run_cmd("gd1");
    expect_running({"pre pre task 1", "pre pre task 2"});
    eout << "\x1B[35mStarting debugger for \"test_suit_1.test1\"...\x1B[0m" << std::endl;
    run_cmd("gd2");
    expect_running({"pre pre task 1", "pre pre task 2"});
    eout << "\x1B[35mStarting debugger for \"test_suit_1.test2\"...\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_execute_non_gtest_task_and_display_output_of_one_of_the_tests_executed_previously) {
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_tests_successfull();
    run_cmd("t1");
    expect_hello_world_task(0);
    run_cmd("g1");
    eout << "\x1B[32m\"test_suit_1.test1\" output:\x1B[0m" << std::endl;
    expect_output_with_links();
    EXPECT_OUTEQ();
}

TEST_F(cdt_test, start_execute_gtest_task_rerun_one_of_its_tests_and_search_output_of_the_rerun_test) {
    EXPECT_CDT_STARTED();
    run_cmd("t8");
    expect_tests_successfull();
    run_cmd("gt1");
    eout << "\x1B[35mRunning \"test_suit_1.test1\"\x1B[0m" << std::endl;
    expect_output_with_links();
    eout << "\x1B[35m'test_suit_1.test1' complete: return code: 0\x1B[0m" << std::endl;
    EXPECT_OUTEQ();
}
