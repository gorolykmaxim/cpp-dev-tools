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

struct paths {
    const std::filesystem::path home = std::filesystem::path("/users/my-user");
    const std::filesystem::path user_config = home / ".cpp-dev-tools.json";
    const std::filesystem::path tasks_config = std::filesystem::absolute("tasks.json");
};

struct executables {
    const std::string cdt = "cpp-dev-tools";
    const std::string editor = "subl";
    const std::string new_terminal_tab = "terminal";
    const std::string debugger = "lldb";
    const std::string hello_world = "echo hello world!";
    const std::string tests = "tests";
};

struct process_exec {
    std::vector<std::string> output_lines;
    std::unordered_set<int> stderr_lines;
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
        for (int i = 0; i < exec.output_lines.size(); i++) {
            std::string& line = exec.output_lines[i];
            if (exec.stderr_lines.count(i) == 0) {
                stdout_cb(line.data(), line.size());
            } else {
                stderr_cb(line.data(), line.size());
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

#define OUT_LINKS_NOT_HIGHLIGHTED()\
    "/a/b/c:10\n"\
    "some random data\n"\
    "/d/e/f:15:32 something\n"\
    "line /a/b/c:11 and /b/c:32:1\n"

#define OUT_LINKS()\
    "\x1B[35m[o1] /a/b/c:10\x1B[0m\n"\
    "some random data\n"\
    "\x1B[35m[o2] /d/e/f:15:32\x1B[0m something\n"\
    "line \x1B[35m[o3] /a/b/c:11\x1B[0m and \x1B[35m[o4] /b/c:32:1\x1B[0m\n"

#define OUT_LIST_OF_TASKS()\
    "\x1B[32mTasks:\x1B[0m\n"\
    "1 \"hello world!\"\n"\
    "2 \"primary task\"\n"\
    "3 \"pre task 1\"\n"\
    "4 \"pre task 2\"\n"\
    "5 \"pre pre task 1\"\n"\
    "6 \"pre pre task 2\"\n"\
    "7 \"restart\"\n"\
    "8 \"run tests\"\n"\
    "9 \"task with gtest pre task\"\n"\
    "10 \"run tests with pre tasks\"\n"

#define OUT_TEST_ERROR()\
    "unknown file: Failure\n"\
    "C++ exception with description \"\" thrown in the test body.\n"

#define OUT_LAST_EXECUTED_TESTS()\
    "\x1B[32mLast executed tests (0 ms total):\x1B[0m\n"\
    "1 \"test_suit_1.test1\" (0 ms)\n"\
    "2 \"test_suit_1.test2\" (0 ms)\n"\
    "3 \"test_suit_2.test1\" (0 ms)\n"

#define OUT_FAILED_TESTS()\
    "\x1B[31mFailed tests:\x1B[0m\n"\
    "1 \"failed_test_suit_1.test1\" (0 ms)\n"\
    "2 \"failed_test_suit_2.test1\" (0 ms)\n"\
    "\x1B[31mTests failed: 2 of 3 (66%) (0 ms total)\x1B[0m\n"

#define OUT_TESTS_EXIT_IN_THE_MIDDLE()\
    "\x1B[2K\r\x1B[31mTests have finished prematurely\x1B[0m\n"\
    "\x1B[31mFailed tests:\x1B[0m\n"\
    "1 \"exit_tests.exit_in_the_middle\" \n"\
    "\x1B[31mTests failed: 1 of 2 (50%) \x1B[0m\n"\
    "\x1B[31m\"exit_tests.exit_in_the_middle\" output:\x1B[0m\n"

#define OUT_TESTS_COMPLETED_SUCCESSFULLY()\
    "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"\
    "\x1B[2K\r\x1B[32mSuccessfully executed 3 tests (0 ms total)\x1B[0m\n"

#define OUT_TESTS_FAILED()\
    "\x1B[2K\r\x1B[31mFailed tests:\x1B[0m\n"\
    "1 \"failed_test_suit_1.test1\" (0 ms)\n"\
    "2 \"failed_test_suit_2.test1\" (0 ms)\n"\
    "\x1B[31mTests failed: 2 of 3 (66%) (0 ms total)\x1B[0m\n"

#define OUT_SINGLE_TEST_FAILED()\
    "\x1B[2K\r\x1B[31mFailed tests:\x1B[0m\n"\
    "1 \"failed_test_suit_1.test1\" (0 ms)\n"\
    "\x1B[31mTests failed: 1 of 2 (50%) (0 ms total)\x1B[0m\n"\
    "\x1B[31m\"failed_test_suit_1.test1\" output:\x1B[0m\n"\
    OUT_LINKS()\
    OUT_TEST_ERROR()

#define EXPECT_OUTEQ(EXPECTED)\
    EXPECT_EQ(EXPECTED, out.str());\
    out.str("")

#define EXPECT_CDT_STARTED()\
    EXPECT_TRUE(init_test_cdt());\
    EXPECT_OUTEQ("Type \x1B[32mh\x1B[0m to see list of all the user commands.\n" OUT_LIST_OF_TASKS())

#define EXPECT_CDT_ABORTED(MSG)\
    EXPECT_FALSE(init_test_cdt());\
    EXPECT_EQ(MSG, out.str())

#define EXPECT_CMD(CMD, OUTPUT)\
    run_cmd(CMD);\
    EXPECT_OUTEQ(std::string("\x1B[32m(cdt) \x1B[0m") + OUTPUT)

#define EXPECT_INTERRUPTED_CMD(CMD, OUTPUT)\
    run_cmd(CMD, true);\
    sigint_handler(SIGINT);\
    exec_cdt_systems(cdt);\
    EXPECT_OUTEQ(std::string("\x1B[32m(cdt) \x1B[0m") + OUTPUT)

#define EXPECT_OUTPUT_LINKS_TO_OPEN()\
    testing::InSequence seq;\
    EXPECT_CALL(mock, exec_process(execs.editor + " /a/b/c:10"));\
    EXPECT_CALL(mock, exec_process(execs.editor + " /d/e/f:15:32"));\
    EXPECT_CALL(mock, exec_process(execs.editor + " /a/b/c:11"));\
    EXPECT_CALL(mock, exec_process(execs.editor + " /b/c:32:1"));\
    run_cmd("o1");\
    run_cmd("o2");\
    run_cmd("o3");\
    run_cmd("o4")

#define EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS()\
    EXPECT_CALL(mock, exec_process(testing::_)).Times(0);\
    std::string expected_header = "\x1B[32mLast execution output:\x1B[0m\n";\
    EXPECT_CMD("o0", expected_header + OUT_LINKS());\
    EXPECT_CMD("o99", expected_header + OUT_LINKS());\
    EXPECT_CMD("o", expected_header + OUT_LINKS())

#define EXPECT_DEBUGGER_CALL(CMD) EXPECT_CALL(mock, exec_process("terminal cd " + paths.tasks_config.parent_path().string() + " && lldb " + CMD))

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

class cdt_test: public testing::Test {
public:
    paths paths;
    executables execs;
    std::string out_links, out_test_error;
    process_exec successful_gtest_exec, failed_gtest_exec, successful_single_gtest_exec, failed_single_gtest_exec, aborted_gtest_exec;
    std::stringstream in, out;
    std::function<void(int)> sigint_handler;
    testing::NiceMock<os_api_mock> mock;
    cdt cdt;

    void SetUp() override {
        cdt.os = &mock;
        EXPECT_CALL(mock, in()).Times(testing::AnyNumber()).WillRepeatedly(testing::ReturnRef(in));
        EXPECT_CALL(mock, out()).Times(testing::AnyNumber()).WillRepeatedly(testing::ReturnRef(out));
        EXPECT_CALL(mock, err()).Times(testing::AnyNumber()).WillRepeatedly(testing::ReturnRef(out));
        EXPECT_CALL(mock, get_env("HOME")).Times(testing::AnyNumber()).WillRepeatedly(testing::Return(paths.home));
        EXPECT_CALL(mock, get_env("LAST_COMMAND")).Times(testing::AnyNumber()).WillRepeatedly(testing::Return(""));
        EXPECT_CALL(mock, get_current_path()).Times(testing::AnyNumber()).WillRepeatedly(testing::Return(paths.tasks_config.parent_path()));
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
        tasks.push_back(create_task("run tests", "__gtest " + execs.tests));
        tasks.push_back(create_task_and_process("task with gtest pre task", {"run tests"}));
        tasks.push_back(create_task("run tests with pre tasks", "__gtest " + execs.tests, {"pre pre task 1", "pre pre task 2"}));
        nlohmann::json tasks_config_data;
        tasks_config_data["cdt_tasks"] = tasks;
        mock.mock_read_file(paths.tasks_config, tasks_config_data.dump());
        // mock user config
        nlohmann::json user_config_data;
        user_config_data["open_in_editor_command"] = execs.editor + " {}";
        user_config_data["execute_in_new_terminal_tab_command"] = execs.new_terminal_tab + " {}";
        user_config_data["debug_command"] = execs.debugger + " {}";
        mock.mock_read_file(paths.user_config, user_config_data.dump());
        EXPECT_CALL(mock, file_exists(paths.user_config)).WillRepeatedly(testing::Return(true));
        // mock default test execution
        out_links = OUT_LINKS_NOT_HIGHLIGHTED();
        out_test_error = OUT_TEST_ERROR();
        successful_gtest_exec.output_lines = {
            "Running main() from /lib/gtest_main.cc\n",
            "[==========] Running 3 tests from 2 test suites.\n",
            "[----------] Global test environment set-up.\n",
            "[----------] 2 tests from test_suit_1\n",
            "[ RUN      ] test_suit_1.test1\n",
            out_links,
            "[       OK ] test_suit_1.test1 (0 ms)\n",
            "[ RUN      ] test_suit_1.test2\n",
            "[       OK ] test_suit_1.test2 (0 ms)\n",
            "[----------] 2 tests from test_suit_1 (0 ms total)\n\n",
            "[----------] 1 test from test_suit_2\n",
            "[ RUN      ] test_suit_2.test1\n",
            "[       OK ] test_suit_2.test1 (0 ms)\n",
            "[----------] 1 test from test_suit_2 (0 ms total)\n\n",
            "[----------] Global test environment tear-down\n",
            "[==========] 3 tests from 2 test suites ran. (0 ms total)\n",
            "[  PASSED  ] 3 tests.\n"
        };
        aborted_gtest_exec.exit_code = 1;
        aborted_gtest_exec.output_lines = {
            "Running main() from /lib/gtest_main.cc\n",
            "[==========] Running 2 tests from 2 test suites.\n",
            "[----------] Global test environment set-up.\n",
            "[----------] 1 test from normal_tests\n",
            "[ RUN      ] normal_tests.hello_world\n",
            "[       OK ] normal_tests.hello_world (0 ms)\n",
            "[----------] 1 test from normal_tests (0 ms total)\n\n",
            "[----------] 1 test from exit_tests\n",
            "[ RUN      ] exit_tests.exit_in_the_middle\n"
        };
        failed_gtest_exec.exit_code = 1;
        failed_gtest_exec.output_lines = {
            "Running main() from /lib/gtest_main.cc\n",
            "[==========] Running 3 tests from 2 test suites.\n",
            "[----------] Global test environment set-up.\n",
            "[----------] 2 tests from failed_test_suit_1\n",
            "[ RUN      ] failed_test_suit_1.test1\n",
            out_links,
            out_test_error,
            "[  FAILED  ] failed_test_suit_1.test1 (0 ms)\n",
            "[ RUN      ] failed_test_suit_1.test2\n",
            "[       OK ] failed_test_suit_1.test2 (0 ms)\n",
            "[----------] 2 tests from failed_test_suit_1 (0 ms total)\n\n",
            "[----------] 1 test from failed_test_suit_2\n",
            "[ RUN      ] failed_test_suit_2.test1\n\n",
            out_test_error,
            "[  FAILED  ] failed_test_suit_2.test1 (0 ms)\n",
            "[----------] 1 test from failed_test_suit_2 (0 ms total)\n\n",
            "[----------] Global test environment tear-down\n",
            "[==========] 3 tests from 2 test suites ran. (0 ms total)\n",
            "[  PASSED  ] 1 test.\n",
            "[  FAILED  ] 2 tests, listed below:\n",
            "[  FAILED  ] failed_test_suit_1.test1\n",
            "[  FAILED  ] failed_test_suit_2.test1\n\n",
            "2 FAILED TESTS\n"
        };
        successful_single_gtest_exec.output_lines = {
            "Running main() from /lib/gtest_main.cc\n",
            "Note: Google Test filter = test_suit_1.test1\n",
            "[==========] Running 1 test from 1 test suite.\n",
            "[----------] Global test environment set-up.\n",
            "[----------] 1 test from test_suit_1\n",
            "[ RUN      ] test_suit_1.test1\n",
            out_links,
            "[       OK ] test_suit_1.test1 (0 ms)\n",
            "[----------] 1 test from test_suit_1 (0 ms total)\n\n",
            "[----------] Global test environment tear-down\n",
            "[==========] 1 test from 1 test suite ran. (0 ms total)\n",
            "[  PASSED  ] 1 test.\n"
        };
        failed_single_gtest_exec.exit_code = 1;
        failed_single_gtest_exec.output_lines = {
            "Running main() from /lib/gtest_main.cc\n",
            "[==========] Running 2 tests from 1 test suite.\n",
            "[----------] Global test environment set-up.\n",
            "[----------] 2 tests from failed_test_suit_1\n",
            "[ RUN      ] failed_test_suit_1.test1\n",
            out_links,
            out_test_error,
            "[  FAILED  ] failed_test_suit_1.test1 (0 ms)\n",
            "[ RUN      ] failed_test_suit_1.test2\n",
            "[       OK ] failed_test_suit_1.test2 (0 ms)\n",
            "[----------] 2 tests from failed_test_suit_1 (0 ms total)\n\n",
            "[----------] Global test environment tear-down\n",
            "[==========] 2 tests from 1 test suite ran. (0 ms total)\n",
            "[  PASSED  ] 1 test.\n",
            "[  FAILED  ] 1 test, listed below:\n",
            "[  FAILED  ] failed_test_suit_1.test1\n\n",
            " 1 FAILED TEST\n"
        };
        mock.cmd_to_process_execs[execs.tests].push_back(successful_gtest_exec);
        mock.cmd_to_process_execs[execs.tests + " --gtest_filter='test_suit_1.test1'"].push_back(successful_single_gtest_exec);
    }
    bool init_test_cdt() {
        std::vector<const char*> argv = {execs.cdt.c_str(), paths.tasks_config.filename().c_str()};
        return init_cdt(argv.size(), argv.data(), cdt);
    }
    nlohmann::json create_task(const nlohmann::json& name = nullptr, const nlohmann::json& command = nullptr, const nlohmann::json& pre_tasks = nullptr) {
        nlohmann::json json;
        if (!name.is_null()) {
            json["name"] = name;
        }
        if (!command.is_null()) {
            json["command"] = command;
        }
        if (!pre_tasks.is_null()) {
            json["pre_tasks"] = pre_tasks;
        }
        return json;
    }
    nlohmann::json create_task_and_process(const std::string& name, std::vector<std::string> pre_tasks = {}) {
        std::string cmd = "echo " + name;
        process_exec exec;
        exec.output_lines = {name + '\n'};
        mock.cmd_to_process_execs[cmd].push_back(exec);
        return create_task(name, cmd, std::move(pre_tasks));
    }
    void run_cmd(const std::string& cmd, bool break_when_process_events_stop = false) {
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
};

TEST_F(cdt_test, start_and_view_tasks) {
    EXPECT_CDT_STARTED();
}

TEST_F(cdt_test, fail_to_start_due_to_user_config_not_being_json) {
    mock.mock_read_file(paths.user_config, "not a json");
    EXPECT_CDT_ABORTED(
        "\x1B[31mFailed to parse " + paths.user_config.string() + ": [json.exception.parse_error.101] parse error at line 1, column 2: syntax error while parsing value - invalid literal; last read: 'no'\n\x1B[0m"
    );
}

TEST_F(cdt_test, fail_to_start_due_to_user_config_having_properties_in_incorrect_format) {
    nlohmann::json user_config_data;
    user_config_data["open_in_editor_command"] = "my-editor";
    user_config_data["execute_in_new_terminal_tab_command"] = "my-terminal";
    user_config_data["debug_command"] = "my-debugger";
    mock.mock_read_file(paths.user_config, user_config_data.dump());
    EXPECT_CDT_ABORTED(
        "\x1B[31m" + paths.user_config.string() + " is invalid:\n"
        "'open_in_editor_command': must be a string in format, examples of which you can find in the config\n"
        "'debug_command': must be a string in format, examples of which you can find in the config\n"
        "'execute_in_new_terminal_tab_command': must be a string in format, examples of which you can find in the config\n\x1B[0m"
    );
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_specified) {
    std::vector<const char*> argv = {execs.cdt.c_str()};
    EXPECT_FALSE(init_cdt(argv.size(), argv.data(), cdt));
    EXPECT_EQ("\x1B[31musage: cpp-dev-tools tasks.json\n\x1B[0m", out.str());
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_existing) {
    mock.mock_read_file(paths.tasks_config);
    EXPECT_CDT_ABORTED("\x1B[31m" + paths.tasks_config.string() + " does not exist\n\x1B[0m");
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_not_being_json) {
    mock.mock_read_file(paths.tasks_config, "not a json");
    EXPECT_CDT_ABORTED(
        "\x1B[31mFailed to parse " + paths.tasks_config.string() + ": [json.exception.parse_error.101] parse error at line 1, column 2: syntax error while parsing value - invalid literal; last read: 'no'\n\x1B[0m"
    );
}

TEST_F(cdt_test, fail_to_start_due_to_cdt_tasks_not_being_specified_in_config) {
    mock.mock_read_file(paths.tasks_config, "{}");
    EXPECT_CDT_ABORTED(
        "\x1B[31m" + paths.tasks_config.string() + " is invalid:\n"
        "'cdt_tasks': must be an array of task objects\n\x1B[0m"
    );
}

TEST_F(cdt_test, fail_to_start_due_to_cdt_tasks_not_being_array_of_objects) {
    nlohmann::json tasks_config_data;
    tasks_config_data["cdt_tasks"] = "string";
    mock.mock_read_file(paths.tasks_config, tasks_config_data.dump());
    EXPECT_CDT_ABORTED(
        "\x1B[31m" + paths.tasks_config.string() + " is invalid:\n"
        "'cdt_tasks': must be an array of task objects\n\x1B[0m"
    );
}

TEST_F(cdt_test, fail_to_start_due_to_tasks_config_having_errors) {
    nlohmann::json tasks_config_data;
    tasks_config_data["cdt_tasks"] = std::vector<nlohmann::json>{
        create_task(nullptr, "command"),
        create_task("name", nullptr, true),
        create_task("name 2", "command", std::vector<std::string>{"non-existent-task"}),
        create_task("cycle-1", "command", std::vector<std::string>{"cycle-2"}),
        create_task("cycle-2", "command", std::vector<std::string>{"cycle-3"}),
        create_task("cycle-3", "command", std::vector<std::string>{"cycle-1"}),
    };
    mock.mock_read_file(paths.tasks_config, tasks_config_data.dump());
    EXPECT_CDT_ABORTED(
        "\x1B[31m" + paths.tasks_config.string() + " is invalid:\n"
        "task #1: 'name': must be a string\n"
        "task #2: 'command': must be a string\n"
        "task #2: 'pre_tasks': must be an array of other task names\n"
        "task #3: references task 'non-existent-task' that does not exist\n"
        "task 'cycle-1' has a circular dependency in it's 'pre_tasks':\n"
        "cycle-1 -> cycle-2 -> cycle-3 -> cycle-1\n"
        "task 'cycle-2' has a circular dependency in it's 'pre_tasks':\n"
        "cycle-2 -> cycle-3 -> cycle-1 -> cycle-2\n"
        "task 'cycle-3' has a circular dependency in it's 'pre_tasks':\n"
        "cycle-3 -> cycle-1 -> cycle-2 -> cycle-3\n\x1B[0m"
    );
}

TEST_F(cdt_test, start_and_display_help) {
    std::string expected_help = "\x1B[32mUser commands:\x1B[0m\n"
        "t<ind>\t\tExecute the task with the specified index\n"
        "tr<ind>\t\tKeep executing the task with the specified index until it fails\n"
        "d<ind>\t\tExecute the task with the specified index with a debugger attached\n"
        "o<ind>\t\tOpen the file link with the specified index in your code editor\n"
        "s\t\tSearch through output of the last executed task with the specified regular expression\n"
        "g<ind>\t\tDisplay output of the specified google test\n"
        "gs<ind>\t\tSearch through output of the specified google test with the specified regular expression\n"
        "gt<ind>\t\tRe-run the google test with the specified index\n"
        "gtr<ind>\tKeep re-running the google test with the specified index until it fails\n"
        "gd<ind>\t\tRe-run the google test with the specified index with debugger attached\n"
        "gf<ind>\t\tRun google tests of the task with the specified index with a specified --gtest_filter\n"
        "h\t\tDisplay list of user commands\n";
    EXPECT_CDT_STARTED();
    // Display help on unknown command
    EXPECT_CMD("zz", expected_help);
    // Display help on explicit command
    EXPECT_CMD("h", expected_help);
}

TEST_F(cdt_test, start_and_display_list_of_tasks_on_task_command) {
    EXPECT_CDT_STARTED();
    // Display tasks list on no index
    EXPECT_CMD("t", OUT_LIST_OF_TASKS());
    // Display tasks list on non-existent task specified
    EXPECT_CMD("t0", OUT_LIST_OF_TASKS());
    EXPECT_CMD("t99", OUT_LIST_OF_TASKS());
}

TEST_F(cdt_test, start_and_execute_single_task) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        "hello world!\n"
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_task_that_prints_to_stdout_and_stderr) {
    process_exec& exec = mock.cmd_to_process_execs[execs.hello_world].front();
    exec.output_lines = {"stdo", "stde", "ut\n", "rr\n"};
    exec.stderr_lines = {1, 3};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        "stdout\n"
        "stderr\n"
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_task_with_pre_tasks_with_pre_tasks) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t2",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"primary task\"\x1B[0m\n"
        "primary task\n"
        "\x1B[35m'primary task' complete: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_fail_primary_task) {
    process_exec& exec = mock.cmd_to_process_execs[execs.hello_world].front();
    exec.exit_code = 1;
    exec.output_lines = {"starting...\n", "error!!!\n"};
    exec.stderr_lines = {1};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        "starting...\n"
        "error!!!\n"
        "\x1B[31m'hello world!' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_fail_one_of_pre_tasks) {
    process_exec& exec = mock.cmd_to_process_execs["echo pre pre task 2"].front();
    exec.exit_code = 1;
    exec.output_lines = {"error!!!\n"};
    exec.stderr_lines = {0};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t2",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "error!!!\n"
        "\x1B[31m'pre pre task 2' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_task_and_abort_it) {
    mock.cmd_to_process_execs[execs.hello_world][0].is_long = true;
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        "hello world!\n"
        "\x1B[31m'hello world!' failed: return code: -1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_exit) {
    EXPECT_CDT_STARTED();
    testing::InSequence seq;
    EXPECT_CALL(mock, signal(SIGINT, SIG_DFL));
    EXPECT_CALL(mock, raise_signal(SIGINT));
    sigint_handler(SIGINT);
}

TEST_F(cdt_test, start_and_change_cwd_to_tasks_configs_directory) {
    EXPECT_CALL(mock, set_current_path(paths.tasks_config.parent_path()));
    EXPECT_CDT_STARTED();
}

TEST_F(cdt_test, start_execute_single_task_and_repeate_the_last_command_on_enter) {
    std::string expected_output = "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        "hello world!\n"
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n";
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1", expected_output);
    EXPECT_CMD("", expected_output);
}

TEST_F(cdt_test, start_and_execute_restart_task) {
    testing::InSequence seq;
    EXPECT_CALL(mock, set_env("LAST_COMMAND", "t7"));
    std::vector<const char*> expected_argv = {execs.cdt.c_str(), paths.tasks_config.c_str(), nullptr};
    EXPECT_CALL(mock, exec(StrVecEq(expected_argv))).WillRepeatedly(testing::Return(0));
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t7", "\x1B[35mRestarting program...\x1B[0m\n");
}

TEST_F(cdt_test, start_and_fail_to_execute_restart_task) {
    std::vector<const char*> expected_argv = {execs.cdt.c_str(), paths.tasks_config.c_str(), nullptr};
    EXPECT_CALL(mock, exec(StrVecEq(expected_argv))).WillRepeatedly(testing::Return(ENOEXEC));
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t7",
        "\x1B[35mRestarting program...\x1B[0m\n"
        "\x1B[31mFailed to restart: Exec format error\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_task_and_open_links_from_output) {
    mock.cmd_to_process_execs[execs.hello_world].front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_fail_to_execute_task_with_links_and_open_links_from_output) {
    process_exec& exec = mock.cmd_to_process_execs[execs.hello_world].front();
    exec.exit_code = 1;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[31m'hello world!' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_fail_to_execute_pre_task_of_task_and_open_links_from_output) {
    process_exec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    exec.exit_code = 1;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t3",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        OUT_LINKS()
        "\x1B[31m'pre pre task 1' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_execute_task_with_links_in_output_attempt_to_open_non_existent_link_and_view_task_output) {
    mock.cmd_to_process_execs[execs.hello_world].front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_fail_to_execute_task_with_links_attempt_to_open_non_existent_link_and_view_task_output) {
    process_exec& exec = mock.cmd_to_process_execs[execs.hello_world].front();
    exec.exit_code = 1;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[31m'hello world!' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_fail_to_execute_pre_task_of_task_attempt_to_open_non_existent_link_and_view_task_output) {
    process_exec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    exec.exit_code = 1;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t3",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        OUT_LINKS()
        "\x1B[31m'pre pre task 1' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(cdt_test, start_attempt_to_open_non_existent_link_and_view_task_output) {
    std::string expected_output = "\x1B[32mNo file links in the output\x1B[0m\n";
    EXPECT_CDT_STARTED();
    EXPECT_CMD("o1", expected_output);
    EXPECT_CMD("o", expected_output);
}

TEST_F(cdt_test, start_attempt_to_open_link_while_open_in_editor_command_is_not_specified_and_see_error) {
    mock.mock_read_file(paths.user_config);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("o", "\x1B[31m'open_in_editor_command' is not specified in \"" + paths.user_config.string() + "\"\x1B[0m\n");
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_no_tests) {
    mock.cmd_to_process_execs[execs.tests].front().output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "[==========] Running 0 tests from 0 test suites.\n",
        "[==========] 0 tests from 0 test suites ran. (0 ms total)\n",
        "[  PASSED  ] 0 tests.\n",
    };
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\x1B[2K\r\x1B[32mSuccessfully executed 0 tests (0 ms total)\x1B[0m\n"
        "\x1B[35m'run tests' complete: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_execute_gtest_task_with_non_existent_binary_and_fail) {
    process_exec& exec = mock.cmd_to_process_execs[execs.tests].front();
    exec.exit_code = 127;
    exec.output_lines = {execs.tests + " does not exist\n"};
    exec.stderr_lines = {0};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\x1B[2K\r\x1B[31m'tests' is not a google test executable\x1B[0m\n"
        "\x1B[31m'run tests' failed: return code: 127\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_non_gtest_binary) {
    mock.cmd_to_process_execs[execs.tests].front() = mock.cmd_to_process_execs[execs.hello_world].front();
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\x1B[2K\r\x1B[31m'tests' is not a google test executable\x1B[0m\n"
        "\x1B[31m'run tests' failed: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_gtest_task_with_non_gtest_binary_that_does_not_finish_and_abort_it_manually) {
    process_exec exec = mock.cmd_to_process_execs[execs.hello_world].front();
    exec.is_long = true;
    mock.cmd_to_process_execs[execs.tests].front() = exec;
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\x1B[2K\r\x1B[31m'tests' is not a google test executable\x1B[0m\n"
        "\x1B[31m'run tests' failed: return code: -1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_gtest_task_that_exits_with_0_exit_code_in_the_middle) {
    aborted_gtest_exec.exit_code = 0;
    mock.cmd_to_process_execs[execs.tests].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 2"
        OUT_TESTS_EXIT_IN_THE_MIDDLE()
        "\x1B[31m'run tests' failed: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_gtest_task_that_exits_with_1_exit_code_in_the_middle) {
    mock.cmd_to_process_execs[execs.tests].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 2"
        OUT_TESTS_EXIT_IN_THE_MIDDLE()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_execute_task_with_gtest_pre_task_that_exits_with_0_exit_code_in_the_middle_and_fail) {
    mock.cmd_to_process_execs[execs.tests].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t9",
        "\x1B[34mRunning \"run tests\"...\x1B[0m\n"
        OUT_TESTS_EXIT_IN_THE_MIDDLE()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_multiple_suites) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests' complete: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_multiple_suites_each_having_failed_tests) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_task_with_gtest_pre_task_with_multiple_suites) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t9",
        "\x1B[34mRunning \"run tests\"...\x1B[0m\n"
        "\x1B[2K\r\x1B[35mRunning \"task with gtest pre task\"\x1B[0m\n"
        "task with gtest pre task\n"
        "\x1B[35m'task with gtest pre task' complete: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_and_execute_task_with_gtest_pre_task_with_multiple_suites_each_having_failed_tests) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t9",
        "\x1B[34mRunning \"run tests\"...\x1B[0m\n"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_gtest_task_with_long_test_and_abort_it) {
    aborted_gtest_exec.is_long = true;
    mock.cmd_to_process_execs[execs.tests].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 2"
        OUT_TESTS_EXIT_IN_THE_MIDDLE()
        "\x1B[31m'run tests' failed: return code: -1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_gtest_task_with_failed_tests_and_long_test_and_abort_it) {
    process_exec& exec = mock.cmd_to_process_execs[execs.tests].front();
    exec.exit_code = 1;
    exec.is_long = true;
    exec.output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "[==========] Running 2 tests from 2 test suites.\n",
        "[----------] Global test environment set-up.\n",
        "[----------] 1 test from failed_test_suit_1\n",
        "[ RUN      ] failed_test_suit_1.test1\n",
        out_links,
        out_test_error,
        "[  FAILED  ] failed_test_suit_1.test1 (0 ms)\n",
        "[----------] 1 test from failed_test_suit_1 (0 ms total)\n\n",
        "[----------] 1 test from long_tests\n",
        "[ RUN      ] long_tests.test1\n"
    };
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 2"
        "\x1B[2K\r\x1B[31mTests have finished prematurely\x1B[0m\n"
        "\x1B[31mFailed tests:\x1B[0m\n"
        "1 \"failed_test_suit_1.test1\" (0 ms)\n"
        "2 \"long_tests.test1\" \n"
        "\x1B[31mTests failed: 2 of 2 (100%) \x1B[0m\n"
        "\x1B[31m'run tests' failed: return code: -1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_repeatedly_execute_task_until_it_fails) {
    std::string tests_success = "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests' complete: return code: 0\x1B[0m\n";
    mock.cmd_to_process_execs[execs.tests].push_back(successful_gtest_exec);
    mock.cmd_to_process_execs[execs.tests].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "tr8",
        tests_success +
        tests_success +
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_repeatedly_execute_task_until_one_of_its_pre_tasks_fails) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "tr9",
        "\x1B[34mRunning \"run tests\"...\x1B[0m\n"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_repeatedly_execute_long_task_and_abort_it) {
    mock.cmd_to_process_execs[execs.hello_world].front().is_long = true;
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD(
        "tr1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        "hello world!\n"
        "\x1B[31m'hello world!' failed: return code: -1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_view_gtest_tests_but_see_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("g", "\x1B[32mNo google tests have been executed yet.\x1B[0m\n");
}

TEST_F(cdt_test, start_execute_gtest_task_try_to_view_gtest_test_output_with_index_out_of_range_and_see_all_tests_list) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD("g0", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("g99", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("g", OUT_LAST_EXECUTED_TESTS());
}

TEST_F(cdt_test, start_execute_gtest_task_view_gtest_task_output_with_file_links_highlighted_in_the_output_and_open_links) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "g1",
        "\x1B[32m\"test_suit_1.test1\" output:\x1B[0m\n"
        OUT_LINKS()
    );
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_execute_gtest_task_fail_try_to_view_gtest_test_output_with_index_out_of_range_and_see_failed_tests_list) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD("g0", OUT_FAILED_TESTS());
    EXPECT_CMD("g99", OUT_FAILED_TESTS());
    EXPECT_CMD("g", OUT_FAILED_TESTS());
}

TEST_F(cdt_test, start_execute_gtest_task_fail_view_gtest_test_output_with_file_links_highlighted_in_the_output_and_open_links) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD(
        "g1",
        "\x1B[31m\"failed_test_suit_1.test1\" output:\x1B[0m\n"
        OUT_LINKS()
        OUT_TEST_ERROR()
    );
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(cdt_test, start_execute_gtest_task_fail_one_of_the_tests_and_view_automatically_displayed_test_output) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_single_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        "\rTests completed: 1 of 2\rTests completed: 2 of 2"
        OUT_SINGLE_TEST_FAILED()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_task_with_gtest_pre_task_fail_one_of_the_tests_and_view_automatically_displayed_test_output) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_single_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t9",
        "\x1B[34mRunning \"run tests\"...\x1B[0m\n"
        OUT_SINGLE_TEST_FAILED()
        "\x1B[31m'run tests' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_rerun_gtest_when_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gt", "\x1B[32mNo google tests have been executed yet.\x1B[0m\n");
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_attempt_to_rerun_test_that_does_not_exist_and_view_list_of_failed_tests) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests with pre tasks' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD("gt", OUT_FAILED_TESTS());
    EXPECT_CMD("gt0", OUT_FAILED_TESTS());
    EXPECT_CMD("gt99", OUT_FAILED_TESTS());
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_and_rerun_failed_test) {
    process_exec first_test_rerun;
    first_test_rerun.exit_code = 1;
    first_test_rerun.output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "Note: Google Test filter = failed_test_suit_1.test1\n",
        "[==========] Running 1 test from 1 test suite.\n",
        "[----------] Global test environment set-up.\n",
        "[----------] 1 test from failed_test_suit_1\n",
        "[ RUN      ] failed_test_suit_1.test1\n",
        out_links,
        out_test_error,
        "[  FAILED  ] failed_test_suit_1.test1 (0 ms)\n",
        "[----------] 1 test from failed_test_suit_1 (0 ms total)\n\n",
        "[----------] Global test environment tear-down\n",
        "[==========] 1 test from 1 test suite ran. (0 ms total)\n",
        "[  PASSED  ] 0 tests.\n",
        "[  FAILED  ] 1 test, listed below:\n",
        "[  FAILED  ] failed_test_suit_1.test1\n\n",
        " 1 FAILED TEST\n",
    };
    process_exec second_test_rerun;
    second_test_rerun.exit_code = 1;
    second_test_rerun.output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "Note: Google Test filter = failed_test_suit_2.test1\n",
        "[==========] Running 1 test from 1 test suite.\n",
        "[----------] Global test environment set-up.\n",
        "[----------] 1 test from failed_test_suit_2\n",
        "[ RUN      ] failed_test_suit_2.test1\n",
        out_test_error,
        "[  FAILED  ] failed_test_suit_2.test1 (0 ms)\n",
        "[----------] 1 test from failed_test_suit_2 (0 ms total)\n\n",
        "[----------] Global test environment tear-down\n",
        "[==========] 1 test from 1 test suite ran. (0 ms total)\n",
        "[  PASSED  ] 0 tests.\n",
        "[  FAILED  ] 1 test, listed below:\n",
        "[  FAILED  ] failed_test_suit_2.test1\n\n",
        " 1 FAILED TEST\n",
    };
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    mock.cmd_to_process_execs[execs.tests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(first_test_rerun);
    mock.cmd_to_process_execs[execs.tests + " --gtest_filter='failed_test_suit_2.test1'"].push_back(second_test_rerun);
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests with pre tasks' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD(
        "gt1",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"failed_test_suit_1.test1\"\x1B[0m\n"
        OUT_LINKS()
        OUT_TEST_ERROR()
        "\x1B[31m'failed_test_suit_1.test1' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD(
        "gt2",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"failed_test_suit_2.test1\"\x1B[0m\n"
        OUT_TEST_ERROR()
        "\x1B[31m'failed_test_suit_2.test1' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_rerun_test_that_does_not_exist_and_view_list_of_all_tests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD("gt", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("gt0", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("gt99", OUT_LAST_EXECUTED_TESTS());
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_and_rerun_one_of_test) {
    process_exec second_test_rerun;
    second_test_rerun.output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "Note: Google Test filter = test_suit_1.test2\n",
        "[==========] Running 1 test from 1 test suite.\n",
        "[----------] Global test environment set-up.\n",
        "[----------] 1 test from test_suit_1\n",
        "[ RUN      ] test_suit_1.test2\n",
        "[       OK ] test_suit_1.test2 (0 ms)\n",
        "[----------] 1 test from test_suit_1 (0 ms total)\n\n",
        "[----------] Global test environment tear-down\n",
        "[==========] 1 test from 1 test suite ran. (0 ms total)\n",
        "[  PASSED  ] 1 test.\n",
    };
    mock.cmd_to_process_execs[execs.tests + " --gtest_filter='test_suit_1.test2'"].push_back(second_test_rerun);
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "gt1",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"test_suit_1.test1\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[35m'test_suit_1.test1' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "gt2",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"test_suit_1.test2\"\x1B[0m\n"
        "\x1B[35m'test_suit_1.test2' complete: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_repeatedly_execute_gtest_task_with_pre_tasks_until_it_fails_and_repeatedly_rerun_failed_test_until_it_fails) {
    process_exec successful_rerun;
    successful_rerun.output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "Note: Google Test filter = failed_test_suit_1.test1\n",
        "[==========] Running 1 test from 1 test suite.\n",
        "[----------] Global test environment set-up.\n",
        "[----------] 1 test from failed_test_suit_1\n",
        "[ RUN      ] failed_test_suit_1.test1\n",
        out_links,
        "[       OK ] failed_test_suit_1.test1 (0 ms)\n",
        "[----------] 1 test from failed_test_suit_1 (0 ms total)\n\n",
        "[----------] Global test environment tear-down\n",
        "[==========] 1 test from 1 test suite ran. (0 ms total)\n",
        "[  PASSED  ] 1 test.\n",
    };
    process_exec failed_rerun;
    failed_rerun.exit_code = 1;
    failed_rerun.output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "Note: Google Test filter = failed_test_suit_1.test1\n",
        "[==========] Running 1 test from 1 test suite.\n",
        "[----------] Global test environment set-up.\n",
        "[----------] 1 test from failed_test_suit_1\n",
        "[ RUN      ] failed_test_suit_1.test1\n",
        out_links,
        out_test_error,
        "[  FAILED  ] failed_test_suit_1.test1 (0 ms)\n",
        "[----------] 1 test from failed_test_suit_1 (0 ms total)\n\n",
        "[----------] Global test environment tear-down\n",
        "[==========] 1 test from 1 test suite ran. (0 ms total)\n",
        "[  PASSED  ] 0 tests.\n",
        "[  FAILED  ] 1 test, listed below:\n",
        "[  FAILED  ] failed_test_suit_1.test1\n\n",
        " 1 FAILED TEST\n",
    };
    mock.cmd_to_process_execs[execs.tests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(successful_rerun);
    mock.cmd_to_process_execs[execs.tests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(successful_rerun);
    mock.cmd_to_process_execs[execs.tests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(failed_rerun);
    mock.cmd_to_process_execs[execs.tests].push_back(successful_gtest_exec);
    mock.cmd_to_process_execs[execs.tests].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    std::string tests_completed = "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n";
    EXPECT_CMD(
        "tr10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n" +
        tests_completed +
        tests_completed +
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests with pre tasks' failed: return code: 1\x1B[0m\n"
    );
    std::string test_completed = "\x1B[35mRunning \"failed_test_suit_1.test1\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[35m'failed_test_suit_1.test1' complete: return code: 0\x1B[0m\n";
    EXPECT_CMD(
        "gtr1",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n" +
        test_completed +
        test_completed +
        "\x1B[35mRunning \"failed_test_suit_1.test1\"\x1B[0m\n"
        OUT_LINKS()
        OUT_TEST_ERROR()
        "\x1B[31m'failed_test_suit_1.test1' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_repeatedly_rerun_one_of_the_tests_and_fail_due_to_failed_pre_task) {
    process_exec failed_pre_task = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    failed_pre_task.exit_code = 1;
    mock.cmd_to_process_execs["echo pre pre task 1"].push_back(failed_pre_task);
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "gtr1",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "pre pre task 1\n"
        "\x1B[31m'pre pre task 1' failed: return code: 1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_long_gtest_task_with_pre_tasks_abort_it_repeatedly_rerun_failed_test_and_abort_it_again) {
    process_exec rerun;
    rerun.exit_code = 1;
    rerun.is_long = true;
    rerun.output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "Note: Google Test filter = exit_tests.exit_in_the_middle\n",
        "[==========] Running 1 test from 1 test suite.\n",
        "[----------] Global test environment set-up.\n",
        "[----------] 1 test from exit_tests\n",
        "[ RUN      ] exit_tests.exit_in_the_middle\n",
    };
    aborted_gtest_exec.is_long = true;
    mock.cmd_to_process_execs[execs.tests].front() = aborted_gtest_exec;
    mock.cmd_to_process_execs[execs.tests + " --gtest_filter='exit_tests.exit_in_the_middle'"].push_back(rerun);
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        "\rTests completed: 1 of 2"
        OUT_TESTS_EXIT_IN_THE_MIDDLE()
        "\x1B[31m'run tests with pre tasks' failed: return code: -1\x1B[0m\n"
    );
    EXPECT_INTERRUPTED_CMD(
        "gtr1",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"exit_tests.exit_in_the_middle\"\x1B[0m\n"
        "\x1B[31m'exit_tests.exit_in_the_middle' failed: return code: -1\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_repeatedly_rerun_gtest_when_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gtr", "\x1B[32mNo google tests have been executed yet.\x1B[0m\n");
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_repeatedly_rerun_test_that_does_not_exist_and_view_list_of_all_tests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD("gtr", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("gtr0", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("gtr99", OUT_LAST_EXECUTED_TESTS());
}

TEST_F(cdt_test, start_and_create_example_user_config) {
    std::string default_user_config_content = "{\n"
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
    EXPECT_CALL(mock, file_exists(paths.user_config)).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(mock, write_file(paths.user_config, default_user_config_content));
    EXPECT_CDT_STARTED();
}

TEST_F(cdt_test, start_and_not_override_existing_user_config) {
    EXPECT_CALL(mock, write_file(testing::Eq(paths.user_config), testing::_)).Times(0);
    EXPECT_CDT_STARTED();
}

TEST_F(cdt_test, start_attempt_to_execute_google_tests_with_filter_targeting_task_that_does_not_exist_and_view_list_of_task) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gf", OUT_LIST_OF_TASKS());
    EXPECT_CMD("gf0", OUT_LIST_OF_TASKS());
    EXPECT_CMD("gf99", OUT_LIST_OF_TASKS());
}

TEST_F(cdt_test, start_and_execute_gtest_task_with_gtest_filter) {
    process_exec filtered_tests;
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
    mock.cmd_to_process_execs[execs.tests + " --gtest_filter='test_suit_1.*'"].push_back(filtered_tests);
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "gf8\ntest_suit_1.*",
        "\x1B[32m--gtest_filter=\x1B[0m"
        "\x1B[35mRunning \"test_suit_1.*\"\x1B[0m\n"
        "\rTests completed: 1 of 2\rTests completed: 2 of 2"
        "\x1B[2K\r\x1B[32mSuccessfully executed 2 tests (0 ms total)\x1B[0m\n"
        "\x1B[35m'test_suit_1.*' complete: return code: 0\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_search_execution_output_but_fail_due_to_no_task_being_executed_before_it) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("s", "\x1B[32mNo task has been executed yet\x1B[0m\n");
}

TEST_F(cdt_test, start_execute_task_attempt_to_search_its_output_with_invalid_regex_and_fail) {
    mock.cmd_to_process_execs[execs.hello_world].front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "s\n[",
        "\x1B[32mRegular expression: \x1B[0m"
        "\x1B[31mInvalid regular expression '[': The expression contained mismatched [ and ].\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_task_search_its_output_and_find_no_results) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        "hello world!\n"
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "s\nnon\\-existent",
        "\x1B[32mRegular expression: \x1B[0m"
        "\x1B[32mNo matches found\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_task_search_its_output_and_find_results) {
    mock.cmd_to_process_execs[execs.hello_world].front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "s\n(some|data)",
        "\x1B[32mRegular expression: \x1B[0m"
        "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
        "\x1B[35m3:\x1B[0m\x1B[35m[o2] /d/e/f:15:32\x1B[0m \x1B[32msome\x1B[0mthing\n"
    );
}

TEST_F(cdt_test, start_attempt_to_search_gtest_output_when_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gs", "\x1B[32mNo google tests have been executed yet.\x1B[0m\n");
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_attempt_to_search_output_of_test_that_does_not_exist_and_view_list_of_failed_tests) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests with pre tasks' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD("gs", OUT_FAILED_TESTS());
    EXPECT_CMD("gs0", OUT_FAILED_TESTS());
    EXPECT_CMD("gs99", OUT_FAILED_TESTS());
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_and_search_output_of_the_test) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests with pre tasks' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD(
        "gs1\n(C\\+\\+|with description|Failure)",
        "\x1B[32mRegular expression: \x1B[0m"
        "\x1B[35m5:\x1B[0munknown file: \x1B[32mFailure\x1B[0m\n"
        "\x1B[35m6:\x1B[0m\x1B[32mC++\x1B[0m exception \x1B[32mwith description\x1B[0m \"\" thrown in the test body.\n"
    );
    EXPECT_CMD(
        "gs2\n(C\\+\\+|with description|Failure)",
        "\x1B[32mRegular expression: \x1B[0m"
        "\x1B[35m2:\x1B[0munknown file: \x1B[32mFailure\x1B[0m\n"
        "\x1B[35m3:\x1B[0m\x1B[32mC++\x1B[0m exception \x1B[32mwith description\x1B[0m \"\" thrown in the test body.\n"
    );
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_search_output_of_test_that_does_not_exist_and_view_list_of_all_tests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD("gs", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("gs0", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("gs99", OUT_LAST_EXECUTED_TESTS());
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_and_search_output_of_one_of_the_tests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "gs1\n(some|data)",
        "\x1B[32mRegular expression: \x1B[0m"
        "\x1B[35m2:\x1B[0m\x1B[32msome\x1B[0m random \x1B[32mdata\x1B[0m\n"
        "\x1B[35m3:\x1B[0m/d/e/f:15:32 \x1B[32msome\x1B[0mthing\n"
    );
    EXPECT_CMD(
        "gs2\n(some|data)",
        "\x1B[32mRegular expression: \x1B[0m"
        "\x1B[32mNo matches found\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_debug_task_while_mandatory_properties_are_not_specified_in_user_config) {
    mock.mock_read_file(paths.user_config);
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "d",
        "\x1B[31m'debug_command' is not specified in \"" + paths.user_config.string() + "\"\x1B[0m\n"
        "\x1B[31m'execute_in_new_terminal_tab_command' is not specified in \"" + paths.user_config.string() + "\"\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_debug_task_that_does_not_exist_and_view_list_of_all_tasks) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("d", OUT_LIST_OF_TASKS());
    EXPECT_CMD("d0", OUT_LIST_OF_TASKS());
    EXPECT_CMD("d99", OUT_LIST_OF_TASKS());
}

TEST_F(cdt_test, start_debug_primary_task_with_pre_tasks) {
    EXPECT_DEBUGGER_CALL("echo primary task");
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "d2",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre task 2\"...\x1B[0m\n"
        "\x1B[35mStarting debugger for \"primary task\"...\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_debug_gtest_primary_task_with_pre_tasks) {
    EXPECT_DEBUGGER_CALL("tests");
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "d10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mStarting debugger for \"run tests with pre tasks\"...\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_rerun_gtest_with_debugger_while_mandatory_properties_are_not_specified_in_user_config) {
    mock.mock_read_file(paths.user_config);
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "gd",
        "\x1B[31m'debug_command' is not specified in \"" + paths.user_config.string() + "\"\x1B[0m\n"
        "\x1B[31m'execute_in_new_terminal_tab_command' is not specified in \"" + paths.user_config.string() + "\"\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_attempt_to_rerun_gtest_with_debugger_when_no_tests_have_been_executed_yet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gd", "\x1B[32mNo google tests have been executed yet.\x1B[0m\n");
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_attempt_to_rerun_test_that_does_not_exist_with_debugger_and_view_list_of_failed_tests) {
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests with pre tasks' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD("gd", OUT_FAILED_TESTS());
    EXPECT_CMD("gd0", OUT_FAILED_TESTS());
    EXPECT_CMD("gd99", OUT_FAILED_TESTS());
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_fail_and_rerun_failed_test_with_debugger) {
    testing::InSequence seq;
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='failed_test_suit_1.test1'");
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='failed_test_suit_2.test1'");
    mock.cmd_to_process_execs[execs.tests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        "\rTests completed: 1 of 3\rTests completed: 2 of 3\rTests completed: 3 of 3"
        OUT_TESTS_FAILED()
        "\x1B[31m'run tests with pre tasks' failed: return code: 1\x1B[0m\n"
    );
    EXPECT_CMD(
        "gd1",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mStarting debugger for \"failed_test_suit_1.test1\"...\x1B[0m\n"
    );
    EXPECT_CMD(
        "gd2",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mStarting debugger for \"failed_test_suit_2.test1\"...\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_attempt_to_rerun_test_that_does_not_exist_with_debugger_and_view_list_of_all_tests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD("gd", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("gd0", OUT_LAST_EXECUTED_TESTS());
    EXPECT_CMD("gd99", OUT_LAST_EXECUTED_TESTS());
}

TEST_F(cdt_test, start_execute_gtest_task_with_pre_tasks_succeed_and_rerun_one_of_tests_with_debugger) {
    testing::InSequence seq;
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='test_suit_1.test1'");
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='test_suit_1.test2'");
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t10",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mRunning \"run tests with pre tasks\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests with pre tasks' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "gd1",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mStarting debugger for \"test_suit_1.test1\"...\x1B[0m\n"
    );
    EXPECT_CMD(
        "gd2",
        "\x1B[34mRunning \"pre pre task 1\"...\x1B[0m\n"
        "\x1B[34mRunning \"pre pre task 2\"...\x1B[0m\n"
        "\x1B[35mStarting debugger for \"test_suit_1.test2\"...\x1B[0m\n"
    );
}

TEST_F(cdt_test, start_execute_gtest_task_execute_non_gtest_task_and_display_output_of_one_of_the_tests_executed_previously) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "t1",
        "\x1B[35mRunning \"hello world!\"\x1B[0m\n"
        "hello world!\n"
        "\x1B[35m'hello world!' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "g1",
        "\x1B[32m\"test_suit_1.test1\" output:\x1B[0m\n"
        OUT_LINKS()
    );
}

TEST_F(cdt_test, start_execute_gtest_task_rerun_one_of_its_tests_and_search_output_of_the_rerun_test) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD(
        "t8",
        "\x1B[35mRunning \"run tests\"\x1B[0m\n"
        OUT_TESTS_COMPLETED_SUCCESSFULLY()
        "\x1B[35m'run tests' complete: return code: 0\x1B[0m\n"
    );
    EXPECT_CMD(
        "gt1",
        "\x1B[35mRunning \"test_suit_1.test1\"\x1B[0m\n"
        OUT_LINKS()
        "\x1B[35m'test_suit_1.test1' complete: return code: 0\x1B[0m\n"
    );
}
