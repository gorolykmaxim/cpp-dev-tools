#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-cardinalities.h>
#include <gmock/gmock-function-mocker.h>
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
#include <filesystem>
#include <fstream>

#include "cdt.h"
#include "json.hpp"
#include "process.hpp"

struct Paths {
    const std::filesystem::path kHome = std::filesystem::path("/users/my-user");
    const std::filesystem::path kUserConfig = kHome / ".cpp-dev-tools.json";
    const std::filesystem::path kTasksConfig = kHome / "project" / "tasks.json";
};

struct Executables {
    const std::string kCdt = "cpp-dev-tools";
    const std::string kEditor = "subl";
    const std::string kNewTerminalTab = "terminal";
    const std::string kDebugger = "lldb";
    const std::string kHelloWorld = "echo hello world!";
    const std::string kTests = "tests";
};

struct ProcessExec {
    std::vector<std::string> output_lines;
    std::unordered_set<int> stderr_lines;
    bool is_long = false;
    int exit_code = 0;
};

struct ProcessExitInfo {
    int exit_code;
    std::function<void()> exit_cb;
};

class OsApiMock: public OsApi {
public:
    MOCK_METHOD(std::istream&, In, (), (override));
    MOCK_METHOD(std::ostream&, Out, (), (override));
    MOCK_METHOD(std::ostream&, Err, (), (override));
    MOCK_METHOD(std::string, GetEnv, (const std::string&), (override));
    MOCK_METHOD(void, SetEnv, (const std::string&, const std::string&), (override));
    MOCK_METHOD(void, SetCurrentPath, (const std::filesystem::path&), (override));
    MOCK_METHOD(std::filesystem::path, GetCurrentPath, (), (override));
    MOCK_METHOD(std::filesystem::path, AbsolutePath, (const std::filesystem::path&), (override));
    MOCK_METHOD(bool, ReadFile, (const std::filesystem::path&, std::string&), (override));
    MOCK_METHOD(void, WriteFile, (const std::filesystem::path&, const std::string&), (override));
    MOCK_METHOD(bool, FileExists, (const std::filesystem::path&), (override));
    MOCK_METHOD(void, Signal, (int, void(*)(int)), (override));
    MOCK_METHOD(void, RaiseSignal, (int), (override));
    MOCK_METHOD(int, Exec, (const std::vector<const char*>&), (override));
    MOCK_METHOD(void, ExecProcess, (const std::string&), (override));
    void KillProcess(Process& process) override {
        ProcessExitInfo& info = proc_exit_info.at(&process);
        info.exit_cb();
        info.exit_code = -1;
    }
    void StartProcess(Process& process,
                      const std::function<void(const char*, size_t)>& stdout_cb,
                      const std::function<void(const char*, size_t)>& stderr_cb,
                      const std::function<void()>& exit_cb) override {
        auto it = cmd_to_process_execs.find(process.shell_command);
        if (it == cmd_to_process_execs.end()) {
            throw std::runtime_error("Unexpected command called: " + process.shell_command);
        }
        process_calls.Call(process.shell_command);
        std::deque<ProcessExec>& execs = it->second;
        ProcessExec exec = execs.front();
        if (execs.size() > 1) {
            execs.pop_front();
        }
        process.handle = std::unique_ptr<TinyProcessLib::Process>();
        proc_exit_info[&process] = ProcessExitInfo{exec.exit_code, exit_cb};
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
    int GetProcessExitCode(Process& process) override {
        return proc_exit_info.at(&process).exit_code;
    }
    std::chrono::system_clock::time_point TimeNow() override {
        return time_now += std::chrono::seconds(1);
    }
    void MockReadFile(const std::filesystem::path& p, const std::string& d) {
        EXPECT_CALL(*this, ReadFile(testing::Eq(p), testing::_)).WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(d), testing::Return(true)));
    }
    void MockReadFile(const std::filesystem::path& p) {
        EXPECT_CALL(*this, ReadFile(testing::Eq(p), testing::_)).WillRepeatedly(testing::Return(false));
    }

    std::chrono::system_clock::time_point time_now;
    std::unordered_map<std::string, std::deque<ProcessExec>> cmd_to_process_execs;
    std::unordered_map<Process*, ProcessExitInfo> proc_exit_info;
    testing::NiceMock<testing::MockFunction<void(const std::string&)>> process_calls;
};

#define OUT_LINKS_NOT_HIGHLIGHTED()\
    "/a/b/c:10\n"\
    "some random data\n"\
    "/d/e/f:15:32 something\n"\
    "line /a/b/c:11 and /b/c:32:1\n"

#define OUT_TEST_ERROR()\
    "unknown file: Failure\n"\
    "C++ exception with description \"\" thrown in the test body.\n"

#define EXPECT_OUT_EQ_SNAPSHOT(NAME)\
    if (std::getenv("SNAPSHOT") != nullptr)\
        mock.OsApi::WriteFile(SnapshotPath(NAME), out.str());\
    else\
        EXPECT_EQ(ReadSnapshot(NAME), out.str());\
    out.str("")

#define EXPECT_CDT_STARTED()\
    EXPECT_TRUE(InitTestCdt());\
    EXPECT_OUT_EQ_SNAPSHOT("InitTestCdt")

#define EXPECT_CDT_ABORTED()\
    EXPECT_FALSE(InitTestCdt());\
    EXPECT_OUT_EQ_SNAPSHOT("")

#define EXPECT_CMD(CMD)\
    RunCmd(CMD);\
    EXPECT_OUT_EQ_SNAPSHOT("")

#define EXPECT_INTERRUPTED_CMD(CMD)\
    RunCmd(CMD, true);\
    sigint_handler(SIGINT);\
    ExecCdtSystems(cdt);\
    EXPECT_OUT_EQ_SNAPSHOT("")

#define EXPECT_OUTPUT_LINKS_TO_OPEN()\
    testing::InSequence seq;\
    EXPECT_CALL(mock, ExecProcess(execs.kEditor + " /a/b/c:10"));\
    EXPECT_CALL(mock, ExecProcess(execs.kEditor + " /d/e/f:15:32"));\
    EXPECT_CALL(mock, ExecProcess(execs.kEditor + " /a/b/c:11"));\
    EXPECT_CALL(mock, ExecProcess(execs.kEditor + " /b/c:32:1"));\
    RunCmd("o1");\
    RunCmd("o2");\
    RunCmd("o3");\
    RunCmd("o4")

#define EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS()\
    EXPECT_CALL(mock, ExecProcess(testing::_)).Times(0);\
    EXPECT_CMD("o0");\
    EXPECT_CMD("o99");\
    EXPECT_CMD("o")

#define DEBUGGER_CALL(CMD) "terminal cd " + paths.kTasksConfig.parent_path().string() + " && lldb " + CMD

#define EXPECT_DEBUGGER_CALL(CMD)\
    mock.cmd_to_process_execs[DEBUGGER_CALL(CMD)].push_back(ProcessExec{});\
    EXPECT_CALL(mock.process_calls, Call(DEBUGGER_CALL(CMD)))

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

class CdtTest: public testing::Test {
public:
    Paths paths;
    Executables execs;
    std::string out_links, out_test_error;
    ProcessExec successful_gtest_exec,
                failed_gtest_exec,
                successful_single_gtest_exec,
                failed_single_gtest_exec,
                aborted_gtest_exec,
                successful_rerun_gtest_exec,
                failed_rerun_gtest_exec,
                failed_debug_exec;
    int expected_data_index;
    std::stringstream in, out;
    std::function<void(int)> sigint_handler;
    testing::NiceMock<OsApiMock> mock;
    Cdt cdt;

    void SetUp() override {
        expected_data_index = 0;
        cdt.os = &mock;
        EXPECT_CALL(mock, In())
            .Times(testing::AnyNumber())
            .WillRepeatedly(testing::ReturnRef(in));
        EXPECT_CALL(mock, Out())
            .Times(testing::AnyNumber())
            .WillRepeatedly(testing::ReturnRef(out));
        EXPECT_CALL(mock, Err())
            .Times(testing::AnyNumber())
            .WillRepeatedly(testing::ReturnRef(out));
        EXPECT_CALL(mock, GetEnv("HOME"))
            .Times(testing::AnyNumber())
            .WillRepeatedly(testing::Return(paths.kHome));
        EXPECT_CALL(mock, GetEnv("LAST_COMMAND"))
            .Times(testing::AnyNumber())
            .WillRepeatedly(testing::Return(""));
        EXPECT_CALL(mock, GetCurrentPath())
            .Times(testing::AnyNumber())
            .WillRepeatedly(testing::Return(paths.kTasksConfig.parent_path()));
        EXPECT_CALL(mock, AbsolutePath(paths.kTasksConfig.filename()))
            .Times(testing::AnyNumber())
            .WillRepeatedly(testing::Return(paths.kTasksConfig));
        EXPECT_CALL(mock, Signal(testing::Eq(SIGINT), testing::_))
            .WillRepeatedly(testing::SaveArg<1>(&sigint_handler));
        EXPECT_CALL(mock.process_calls, Call(testing::_))
            .Times(testing::AnyNumber());
        // mock tasks config
        std::vector<nlohmann::json> tasks;
        tasks.push_back(CreateTaskAndProcess("hello world!"));
        tasks.push_back(CreateTaskAndProcess("primary task", {"pre task 1", "pre task 2"}));
        tasks.push_back(CreateTaskAndProcess("pre task 1", {"pre pre task 1", "pre pre task 2"}));
        tasks.push_back(CreateTaskAndProcess("pre task 2"));
        tasks.push_back(CreateTaskAndProcess("pre pre task 1"));
        tasks.push_back(CreateTaskAndProcess("pre pre task 2"));
        tasks.push_back(CreateTask("restart", "__restart"));
        tasks.push_back(CreateTask("run tests", "__gtest " + execs.kTests));
        tasks.push_back(CreateTaskAndProcess("task with gtest pre task", {"run tests"}));
        tasks.push_back(CreateTask("run tests with pre tasks", "__gtest " + execs.kTests, {"pre pre task 1", "pre pre task 2"}));
        nlohmann::json tasks_config_data;
        tasks_config_data["cdt_tasks"] = tasks;
        mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
        // mock user config
        nlohmann::json user_config_data;
        user_config_data["open_in_editor_command"] = execs.kEditor + " {}";
        user_config_data["debug_command"] = execs.kNewTerminalTab +
                                            " cd {current_dir} && " +
                                            execs.kDebugger + " {shell_cmd}";
        mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
        EXPECT_CALL(mock, FileExists(paths.kUserConfig)).WillRepeatedly(testing::Return(true));
        // mock default test execution
        out_links = OUT_LINKS_NOT_HIGHLIGHTED();
        out_test_error = "unknown file: Failure\n"
            "C++ exception with description \"\" thrown in the test body.\n";
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
            "[ RUN      ] exit_tests.exit_in_the_middle\n",
            out_test_error
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
        successful_rerun_gtest_exec.output_lines = {
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
        failed_rerun_gtest_exec.exit_code = 1;
        failed_rerun_gtest_exec.output_lines = {
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
        failed_debug_exec.exit_code = 1;
        failed_debug_exec.output_lines = {"failed to launch debugger\n"};
        failed_debug_exec.stderr_lines.insert(0);
        mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
        mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='test_suit_1.test1'"].push_back(successful_single_gtest_exec);
    }
    bool InitTestCdt() {
        std::vector<const char*> argv = {execs.kCdt.c_str(), paths.kTasksConfig.filename().c_str()};
        return InitCdt(argv.size(), argv.data(), cdt);
    }
    nlohmann::json CreateTask(const nlohmann::json& name = nullptr, const nlohmann::json& command = nullptr, const nlohmann::json& pre_tasks = nullptr) {
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
    nlohmann::json CreateTaskAndProcess(const std::string& name, std::vector<std::string> pre_tasks = {}) {
        std::string cmd = "echo " + name;
        ProcessExec exec;
        exec.output_lines = {name + '\n'};
        mock.cmd_to_process_execs[cmd].push_back(exec);
        return CreateTask(name, cmd, std::move(pre_tasks));
    }
    void RunCmd(const std::string& cmd, bool break_when_process_events_stop = false) {
        in << cmd << std::endl;
        while (true) {
            ExecCdtSystems(cdt);
            if (!break_when_process_events_stop && WillWaitForInput(cdt)) {
                break;
            }
            if (break_when_process_events_stop && cdt.proc_event_queue.size_approx() == 0 && cdt.execs_to_run.empty() && cdt.registry.view<Running>().size() == 1) {
                break;
            }
        }
    }
    std::filesystem::path SnapshotPath(std::string name) {
        testing::UnitTest* test = testing::UnitTest::GetInstance();
        std::filesystem::path snapshot_path(TEST_DATA_DIR);
        if (name.empty()) {
            const testing::TestInfo* info = test->current_test_info();
            std::string suite_name = info->test_suite_name();
            std::string test_name = info->name();
            std::string data_ind = std::to_string(expected_data_index++);
            name = suite_name + '.' + test_name + data_ind;
        }
        snapshot_path /= name + ".txt";
        return snapshot_path;
    }
    std::string ReadSnapshot(const std::string& name) {
        std::string snapshot;
        mock.OsApi::ReadFile(SnapshotPath(name), snapshot);
        return snapshot;
    }
};

TEST_F(CdtTest, StartAndViewTasks) {
    EXPECT_CDT_STARTED();
}

TEST_F(CdtTest, FailToStartDueToUserConfigNotBeingJson) {
    mock.MockReadFile(paths.kUserConfig, "not a json");
    EXPECT_CDT_ABORTED();
}

TEST_F(CdtTest, FailToStartDueToUserConfigHavingPropertiesInIncorrectFormat) {
    nlohmann::json user_config_data;
    user_config_data["open_in_editor_command"] = "my-editor";
    user_config_data["debug_command"] = "my-debugger";
    mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
    EXPECT_CDT_ABORTED();
}

TEST_F(CdtTest, FailToStartDueToTasksConfigNotSpecified) {
    std::vector<const char*> argv = {execs.kCdt.c_str()};
    EXPECT_FALSE(InitCdt(argv.size(), argv.data(), cdt));
    EXPECT_EQ("\x1B[31musage: cpp-dev-tools tasks.json\n\x1B[0m", out.str());
}

TEST_F(CdtTest, FailToStartDueToTasksConfigNotExisting) {
    mock.MockReadFile(paths.kTasksConfig);
    EXPECT_CDT_ABORTED();
}

TEST_F(CdtTest, FailToStartDueToTasksConfigNotBeingJson) {
    mock.MockReadFile(paths.kTasksConfig, "not a json");
    EXPECT_CDT_ABORTED();
}

TEST_F(CdtTest, FailToStartDueToCdtTasksNotBeingSpecifiedInConfig) {
    mock.MockReadFile(paths.kTasksConfig, "{}");
    EXPECT_CDT_ABORTED();
}

TEST_F(CdtTest, FailToStartDueToCdtTasksNotBeingArrayOfObjects) {
    nlohmann::json tasks_config_data;
    tasks_config_data["cdt_tasks"] = "string";
    mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
    EXPECT_CDT_ABORTED();
}

TEST_F(CdtTest, FailToStartDueToTasksConfigHavingErrors) {
    nlohmann::json tasks_config_data;
    tasks_config_data["cdt_tasks"] = std::vector<nlohmann::json>{
        CreateTask(nullptr, "command"),
        CreateTask("name", nullptr, true),
        CreateTask("name 2", "command", std::vector<std::string>{"non-existent-task"}),
        CreateTask("cycle-1", "command", std::vector<std::string>{"cycle-2"}),
        CreateTask("cycle-2", "command", std::vector<std::string>{"cycle-3"}),
        CreateTask("cycle-3", "command", std::vector<std::string>{"cycle-1"}),
    };
    mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
    EXPECT_CDT_ABORTED();
}

TEST_F(CdtTest, StartAndDisplayHelp) {
    std::string expected_help = "\x1B[32mUser commands:\x1B[0m\n"
        "t<ind>\t\tExecute the task with the specified index\n"
        "tr<ind>\t\tKeep executing the task with the specified index until it fails\n"
        "d<ind>\t\tExecute the task with the specified index with a debugger attached\n"
        "exec<ind>\tChange currently selected execution (gets reset to the most recent one after every new execution)\n"
        "o<ind>\t\tOpen the file link with the specified index in your code editor\n"
        "s\t\tSearch through output of the selected executed task with the specified regular expression\n"
        "g<ind>\t\tDisplay output of the specified google test\n"
        "gs<ind>\t\tSearch through output of the specified google test with the specified regular expression\n"
        "gt<ind>\t\tRe-run the google test with the specified index\n"
        "gtr<ind>\tKeep re-running the google test with the specified index until it fails\n"
        "gd<ind>\t\tRe-run the google test with the specified index with debugger attached\n"
        "gf<ind>\t\tRun google tests of the task with the specified index with a specified --gtest_filter\n"
        "h\t\tDisplay list of user commands\n";
    EXPECT_CDT_STARTED();
    // Display help on unknown command
    EXPECT_CMD("zz");
    // Display help on explicit command
    EXPECT_CMD("h");
}

TEST_F(CdtTest, StartAndDisplayListOfTasksOnTaskCommand) {
    EXPECT_CDT_STARTED();
    // Display tasks list on no index
    EXPECT_CMD("t");
    // Display tasks list on non-existent task specified
    EXPECT_CMD("t0");
    EXPECT_CMD("t99");
}

TEST_F(CdtTest, StartAndExecuteSingleTask) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
}

TEST_F(CdtTest, StartAndExecuteTaskThatPrintsToStdoutAndStderr) {
    ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
    exec.output_lines = {"stdo", "stde", "ut\n", "rr\n"};
    exec.stderr_lines = {1, 3};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
}

TEST_F(CdtTest, StartAndExecuteTaskWithPreTasksWithPreTasks) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t2");
}

TEST_F(CdtTest, StartAndFailPrimaryTask) {
    ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
    exec.exit_code = 1;
    exec.output_lines = {"starting...\n", "error!!!\n"};
    exec.stderr_lines = {1};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
}

TEST_F(CdtTest, StartAndFailOneOfPreTasks) {
    ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 2"].front();
    exec.exit_code = 1;
    exec.output_lines = {"error!!!\n"};
    exec.stderr_lines = {0};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t2");
}

TEST_F(CdtTest, StartExecuteTaskAndAbortIt) {
    mock.cmd_to_process_execs[execs.kHelloWorld][0].is_long = true;
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD("t1");
}

TEST_F(CdtTest, StartAndExit) {
    EXPECT_CDT_STARTED();
    testing::InSequence seq;
    EXPECT_CALL(mock, Signal(SIGINT, SIG_DFL));
    EXPECT_CALL(mock, RaiseSignal(SIGINT));
    sigint_handler(SIGINT);
}

TEST_F(CdtTest, StartAndChangeCwdToTasksConfigsDirectory) {
    EXPECT_CALL(mock, SetCurrentPath(paths.kTasksConfig.parent_path()));
    EXPECT_CDT_STARTED();
}

TEST_F(CdtTest, StartExecuteSingleTaskAndRepeateTheLastCommandOnEnter) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_CMD("");
}

TEST_F(CdtTest, StartAndExecuteRestartTask) {
    testing::InSequence seq;
    EXPECT_CALL(mock, SetEnv("LAST_COMMAND", "t7"));
    std::vector<const char*> expected_argv = {execs.kCdt.c_str(), paths.kTasksConfig.c_str(), nullptr};
    EXPECT_CALL(mock, Exec(StrVecEq(expected_argv))).WillRepeatedly(testing::Return(0));
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t7");
}

TEST_F(CdtTest, StartAndFailToExecuteRestartTask) {
    std::vector<const char*> expected_argv = {execs.kCdt.c_str(), paths.kTasksConfig.c_str(), nullptr};
    EXPECT_CALL(mock, Exec(StrVecEq(expected_argv))).WillRepeatedly(testing::Return(ENOEXEC));
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t7");
}

TEST_F(CdtTest, StartExecuteTaskAndOpenLinksFromOutput) {
    mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(CdtTest, StartFailToExecuteTaskWithLinksAndOpenLinksFromOutput) {
    ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
    exec.exit_code = 1;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(CdtTest, StartFailToExecutePreTaskOfTaskAndOpenLinksFromOutput) {
    ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    exec.exit_code = 1;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t3");
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(CdtTest, StartExecuteTaskWithLinksInOutputAttemptToOpenNonExistentLinkAndViewTaskOutput) {
    mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(CdtTest, StartFailToExecuteTaskWithLinksAttemptToOpenNonExistentLinkAndViewTaskOutput) {
    ProcessExec& exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
    exec.exit_code = 1;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(CdtTest, StartFailToExecutePreTaskOfTaskAttemptToOpenNonExistentLinkAndViewTaskOutput) {
    ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    exec.exit_code = 1;
    exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t3");
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
}

TEST_F(CdtTest, StartAttemptToOpenNonExistentLinkAndViewTaskOutput) {
    std::string expected_output = "\x1B[32mNo file links in the output\x1B[0m\n";
    EXPECT_CDT_STARTED();
    EXPECT_CMD("o1");
    EXPECT_CMD("o");
}

TEST_F(CdtTest, StartAttemptToOpenLinkWhileOpenInEditorCommandIsNotSpecifiedAndSeeError) {
    mock.MockReadFile(paths.kUserConfig);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("o");
}

TEST_F(CdtTest, StartAndExecuteGtestTaskWithNoTests) {
    mock.cmd_to_process_execs[execs.kTests].front().output_lines = {
        "Running main() from /lib/gtest_main.cc\n",
        "[==========] Running 0 tests from 0 test suites.\n",
        "[==========] 0 tests from 0 test suites ran. (0 ms total)\n",
        "[  PASSED  ] 0 tests.\n",
    };
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
}

TEST_F(CdtTest, StartAttemptToExecuteGtestTaskWithNonExistentBinaryAndFail) {
    ProcessExec& exec = mock.cmd_to_process_execs[execs.kTests].front();
    exec.exit_code = 127;
    exec.output_lines = {execs.kTests + " does not exist\n"};
    exec.stderr_lines = {0};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
}

TEST_F(CdtTest, StartAndExecuteGtestTaskWithNonGtestBinary) {
    mock.cmd_to_process_execs[execs.kTests].front() = mock.cmd_to_process_execs[execs.kHelloWorld].front();
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithNonGtestBinaryThatDoesNotFinishAndAbortItManually) {
    ProcessExec exec = mock.cmd_to_process_execs[execs.kHelloWorld].front();
    exec.is_long = true;
    mock.cmd_to_process_execs[execs.kTests].front() = exec;
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD("t8");
}

TEST_F(CdtTest, StartAndExecuteGtestTaskThatExitsWith0ExitCodeInTheMiddle) {
    aborted_gtest_exec.exit_code = 0;
    mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
}

TEST_F(CdtTest, StartAndExecuteGtestTaskThatExitsWith1ExitCodeInTheMiddle) {
    mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
}

TEST_F(CdtTest, StartAttemptToExecuteTaskWithGtestPreTaskThatExitsWith0ExitCodeInTheMiddleAndFail) {
    mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t9");
}

TEST_F(CdtTest, StartAndExecuteGtestTaskWithMultipleSuites) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
}

TEST_F(CdtTest, StartAndExecuteGtestTaskWithMultipleSuitesEachHavingFailedTests) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
}

TEST_F(CdtTest, StartAndExecuteTaskWithGtestPreTaskWithMultipleSuites) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t9");
}

TEST_F(CdtTest, StartAndExecuteTaskWithGtestPreTaskWithMultipleSuitesEachHavingFailedTests) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t9");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithLongTestAndAbortIt) {
    aborted_gtest_exec.is_long = true;
    mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD("t8");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithFailedTestsAndLongTestAndAbortIt) {
    ProcessExec& exec = mock.cmd_to_process_execs[execs.kTests].front();
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
    EXPECT_INTERRUPTED_CMD("t8");
}

TEST_F(CdtTest, StartRepeatedlyExecuteTaskUntilItFails) {
    mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
    mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("tr8");
}

TEST_F(CdtTest, StartRepeatedlyExecuteTaskUntilOneOfItsPreTasksFails) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("tr9");
}

TEST_F(CdtTest, StartRepeatedlyExecuteLongTaskAndAbortIt) {
    mock.cmd_to_process_execs[execs.kHelloWorld].front().is_long = true;
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD("tr1");
}

TEST_F(CdtTest, StartAttemptToViewGtestTestsButSeeNoTestsHaveBeenExecutedYet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("g");
}

TEST_F(CdtTest, StartExecuteGtestTaskTryToViewGtestTestOutputWithIndexOutOfRangeAndSeeAllTestsList) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("g0");
    EXPECT_CMD("g99");
    EXPECT_CMD("g");
}

TEST_F(CdtTest, StartExecuteGtestTaskViewGtestTaskOutputWithFileLinksHighlightedInTheOutputAndOpenLinks) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("g1");
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(CdtTest, StartExecuteGtestTaskFailTryToViewGtestTestOutputWithIndexOutOfRangeAndSeeFailedTestsList) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("g0");
    EXPECT_CMD("g99");
    EXPECT_CMD("g");
}

TEST_F(CdtTest, StartExecuteGtestTaskFailViewGtestTestOutputWithFileLinksHighlightedInTheOutputAndOpenLinks) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("g1");
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(CdtTest, StartExecuteGtestTaskFailOneOfTheTestsAndViewAutomaticallyDisplayedTestOutput) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_single_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
}

TEST_F(CdtTest, StartExecuteTaskWithGtestPreTaskFailOneOfTheTestsAndViewAutomaticallyDisplayedTestOutput) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_single_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t9");
}

TEST_F(CdtTest, StartAttemptToRerunGtestWhenNoTestsHaveBeenExecutedYet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gt");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksFailAttemptToRerunTestThatDoesNotExistAndViewListOfFailedTests) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gt");
    EXPECT_CMD("gt0");
    EXPECT_CMD("gt99");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksFailAndRerunFailedTest) {
    ProcessExec first_test_rerun = failed_rerun_gtest_exec;
    ProcessExec second_test_rerun;
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
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(first_test_rerun);
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='failed_test_suit_2.test1'"].push_back(second_test_rerun);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gt1");
    EXPECT_CMD("gt2");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRerunTestThatDoesNotExistAndViewListOfAllTests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gt");
    EXPECT_CMD("gt0");
    EXPECT_CMD("gt99");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksSucceedAndRerunOneOfTest) {
    ProcessExec second_test_rerun;
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
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='test_suit_1.test2'"].push_back(second_test_rerun);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gt1");
    EXPECT_CMD("gt2");
}

TEST_F(CdtTest, StartRepeatedlyExecuteGtestTaskWithPreTasksUntilItFailsAndRepeatedlyRerunFailedTestUntilItFails) {
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(successful_rerun_gtest_exec);
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(successful_rerun_gtest_exec);
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(failed_rerun_gtest_exec);
    mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
    mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("tr10");
    EXPECT_CMD("gtr1");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksSucceedRepeatedlyRerunOneOfTheTestsAndFailDueToFailedPreTask) {
    ProcessExec failed_pre_task = mock.cmd_to_process_execs["echo pre pre task 1"].front();
    failed_pre_task.exit_code = 1;
    mock.cmd_to_process_execs["echo pre pre task 1"].push_back(failed_pre_task);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gtr1");
}

TEST_F(CdtTest, StartExecuteLongGtestTaskWithPreTasksAbortItRepeatedlyRerunFailedTestAndAbortItAgain) {
    ProcessExec rerun;
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
    mock.cmd_to_process_execs[execs.kTests].front() = aborted_gtest_exec;
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='exit_tests.exit_in_the_middle'"].push_back(rerun);
    EXPECT_CDT_STARTED();
    EXPECT_INTERRUPTED_CMD("t10");
    EXPECT_INTERRUPTED_CMD("gtr1");
}

TEST_F(CdtTest, StartAttemptToRepeatedlyRerunGtestWhenNoTestsHaveBeenExecutedYet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gtr");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRepeatedlyRerunTestThatDoesNotExistAndViewListOfAllTests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gtr");
    EXPECT_CMD("gtr0");
    EXPECT_CMD("gtr99");
}

TEST_F(CdtTest, StartAndCreateExampleUserConfig) {
    std::string default_user_config_content = "{\n"
        "  // Open file links from the output in Sublime Text:\n"
        "  //\"open_in_editor_command\": \"subl {}\"\n"
        "  // Open file links from the output in VSCode:\n"
        "  //\"open_in_editor_command\": \"code {}\"\n"
        "  // Debug tasks on MacOS:\n"
        "  //\"debug_command\": \"osascript -e 'tell application \\\"Terminal\\\" to do script \\\"cd {current_dir} && lldb -- {shell_cmd}\\\"'\"\n"
        "}\n";
    EXPECT_CALL(mock, FileExists(paths.kUserConfig)).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(mock, WriteFile(paths.kUserConfig, default_user_config_content));
    EXPECT_CDT_STARTED();
}

TEST_F(CdtTest, StartAndNotOverrideExistingUserConfig) {
    EXPECT_CALL(mock, WriteFile(testing::Eq(paths.kUserConfig), testing::_)).Times(0);
    EXPECT_CDT_STARTED();
}

TEST_F(CdtTest, StartAttemptToExecuteGoogleTestsWithFilterTargetingTaskThatDoesNotExistAndViewListOfTask) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gf");
    EXPECT_CMD("gf0");
    EXPECT_CMD("gf99");
}

TEST_F(CdtTest, StartAndExecuteGtestTaskWithGtestFilter) {
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
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='test_suit_1.*'"].push_back(filtered_tests);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gf8\ntest_suit_1.*");
}

TEST_F(CdtTest, StartAttemptToSearchExecutionOutputButFailDueToNoTaskBeingExecutedBeforeIt) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("s");
}

TEST_F(CdtTest, StartExecuteTaskAttemptToSearchItsOutputWithInvalidRegexAndFail) {
    mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_CMD("s\n[");
}

TEST_F(CdtTest, StartExecuteTaskSearchItsOutputAndFindNoResults) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_CMD("s\nnon\\-existent");
}

TEST_F(CdtTest, StartExecuteTaskSearchItsOutputAndFindResults) {
    mock.cmd_to_process_execs[execs.kHelloWorld].front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_CMD("s\n(some|data)");
}

TEST_F(CdtTest, StartAttemptToSearchGtestOutputWhenNoTestsHaveBeenExecutedYet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gs");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksFailAttemptToSearchOutputOfTestThatDoesNotExistAndViewListOfFailedTests) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gs");
    EXPECT_CMD("gs0");
    EXPECT_CMD("gs99");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksFailAndSearchOutputOfTheTest) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gs1\n(C\\+\\+|with description|Failure)");
    EXPECT_CMD("gs2\n(C\\+\\+|with description|Failure)");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToSearchOutputOfTestThatDoesNotExistAndViewListOfAllTests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gs");
    EXPECT_CMD("gs0");
    EXPECT_CMD("gs99");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksSucceedAndSearchOutputOfOneOfTheTests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gs1\n(some|data)");
    EXPECT_CMD("gs2\n(some|data)");
}

TEST_F(CdtTest, StartAttemptToDebugTaskWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
    mock.MockReadFile(paths.kUserConfig);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("d");
}

TEST_F(CdtTest, StartAttemptDebugTaskThatDoesNotExistAndViewListOfAllTasks) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("d");
    EXPECT_CMD("d0");
    EXPECT_CMD("d99");
}

TEST_F(CdtTest, StartDebugPrimaryTaskWithPreTasks) {
    EXPECT_DEBUGGER_CALL("echo primary task");
    EXPECT_CDT_STARTED();
    EXPECT_CMD("d2");
}

TEST_F(CdtTest, StartAndFailToDebugTask) {
    std::string debugger_call = DEBUGGER_CALL("echo primary task");
    mock.cmd_to_process_execs[debugger_call].push_back(failed_debug_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("d2");
}

TEST_F(CdtTest, StartDebugGtestPrimaryTaskWithPreTasks) {
    EXPECT_DEBUGGER_CALL("tests");
    EXPECT_CDT_STARTED();
    EXPECT_CMD("d10");
}

TEST_F(CdtTest, StartAndFailToDebugGtestTask) {
    std::string debugger_call = DEBUGGER_CALL("tests");
    mock.cmd_to_process_execs[debugger_call].push_back(failed_debug_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("d10");
}

TEST_F(CdtTest, StartAttemptToRerunGtestWithDebuggerWhileMandatoryPropertiesAreNotSpecifiedInUserConfig) {
    mock.MockReadFile(paths.kUserConfig);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gd");
}

TEST_F(CdtTest, StartAttemptToRerunGtestWithDebuggerWhenNoTestsHaveBeenExecutedYet) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("gd");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksFailAttemptToRerunTestThatDoesNotExistWithDebuggerAndViewListOfFailedTests) {
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gd");
    EXPECT_CMD("gd0");
    EXPECT_CMD("gd99");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksFailAndRerunFailedTestWithDebugger) {
    testing::InSequence seq;
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='failed_test_suit_1.test1'");
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='failed_test_suit_2.test1'");
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gd1");
    EXPECT_CMD("gd2");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksSucceedAttemptToRerunTestThatDoesNotExistWithDebuggerAndViewListOfAllTests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gd");
    EXPECT_CMD("gd0");
    EXPECT_CMD("gd99");
}

TEST_F(CdtTest, StartExecuteGtestTaskWithPreTasksSucceedAndRerunOneOfTestsWithDebugger) {
    testing::InSequence seq;
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='test_suit_1.test1'");
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='test_suit_1.test2'");
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t10");
    EXPECT_CMD("gd1");
    EXPECT_CMD("gd2");
}

TEST_F(CdtTest, StartExecuteGtestTaskExecuteNonGtestTaskAndDisplayOutputOfOneOfTheTestsExecutedPreviously) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("t1");
    EXPECT_CMD("g1");
}

TEST_F(CdtTest, StartExecuteGtestTaskSearchItsRawOutput) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("s\n(some|data)");
}

TEST_F(CdtTest, StartExecuteGtestTaskRerunOneOfItsTestsAndDisplayListOfOriginallyExecutedTests) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("gt1");
    EXPECT_CMD("g");
}

TEST_F(CdtTest, StartAndListExecutions) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("exec");
}

TEST_F(CdtTest, StartExecuteTwoTasksAndListExecutions) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_CMD("t1");
    EXPECT_CMD("exec");
    EXPECT_CMD("exec0");
    EXPECT_CMD("exec99");
}

TEST_F(CdtTest, StartExecuteTwoTasksSelectFirstExecutionAndOpenFileLinksFromIt) {
    std::deque<ProcessExec>& proc_execs = mock.cmd_to_process_execs[execs.kHelloWorld];
    proc_execs.push_back(proc_execs.front());
    proc_execs.front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_CMD("t1");
    EXPECT_CMD("exec2");
    EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS();
    EXPECT_OUTPUT_LINKS_TO_OPEN();
}

TEST_F(CdtTest, StartExecuteTwoTasksSelectFirstExecutionAndSearchItsOutput) {
    std::deque<ProcessExec>& proc_execs = mock.cmd_to_process_execs[execs.kHelloWorld];
    proc_execs.push_back(proc_execs.front());
    proc_execs.front().output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_CMD("t1");
    EXPECT_CMD("exec2");
    EXPECT_CMD("s\n(some|data)");
}

TEST_F(CdtTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndViewGtestOutput) {
    mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("t8");
    EXPECT_CMD("exec2");
    EXPECT_CMD("g0");
    EXPECT_CMD("g99");
    EXPECT_CMD("g");
    EXPECT_CMD("g1");
}

TEST_F(CdtTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndSearchGtestOutput) {
    mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("t8");
    EXPECT_CMD("exec2");
    EXPECT_CMD("gs");
    EXPECT_CMD("gs0");
    EXPECT_CMD("gs99");
    EXPECT_CMD("gs1\n(some|data)");
}

TEST_F(CdtTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtest) {
    mock.cmd_to_process_execs[execs.kTests].push_back(failed_gtest_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("t8");
    EXPECT_CMD("exec2");
    EXPECT_CMD("gt");
    EXPECT_CMD("gt0");
    EXPECT_CMD("gt99");
    EXPECT_CMD("gt1");
}

TEST_F(CdtTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtestUntilItFails) {
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(successful_rerun_gtest_exec);
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(successful_rerun_gtest_exec);
    mock.cmd_to_process_execs[execs.kTests + " --gtest_filter='failed_test_suit_1.test1'"].push_back(failed_rerun_gtest_exec);
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("t8");
    EXPECT_CMD("exec2");
    EXPECT_CMD("gtr");
    EXPECT_CMD("gtr0");
    EXPECT_CMD("gtr99");
    EXPECT_CMD("gtr1");
}

TEST_F(CdtTest, StartExecuteTwoGtestTasksSelectFirstExecutionAndRerunGtestWithDebugger) {
    EXPECT_DEBUGGER_CALL("tests --gtest_filter='failed_test_suit_1.test1'");
    mock.cmd_to_process_execs[execs.kTests].front() = failed_gtest_exec;
    mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t8");
    EXPECT_CMD("t8");
    EXPECT_CMD("exec2");
    EXPECT_CMD("gd");
    EXPECT_CMD("gd0");
    EXPECT_CMD("gd99");
    EXPECT_CMD("gd1");
}

TEST_F(CdtTest, StartExecuteTaskTwiceSelectFirstExecutionExecuteAnotherTaskSeeFirstTaskStillSelectedSelectLastExecutionExecuteAnotherTaskAndSeeItBeingSelectedAutomatically) {
    EXPECT_CDT_STARTED();
    EXPECT_CMD("t1");
    EXPECT_CMD("t1");
    EXPECT_CMD("exec2");
    EXPECT_CMD("exec");
    EXPECT_CMD("t1");
    EXPECT_CMD("exec");
    EXPECT_CMD("exec1");
    EXPECT_CMD("t1");
    EXPECT_CMD("exec");
}

TEST_F(CdtTest, StartExecuteGtestTaskTwiceExecuteNormalTask110TimesWhileSelectingFirstNormalExecutionAndViewHistoryOf100ExecutionsThatIncludesOnlyOneLastGtestTaskAndSelectedFirstNormalExecution) {
    EXPECT_CDT_STARTED();
    for (int i = 0; i < 2; i++) {
        EXPECT_CMD("t8");
    }
    for (int i = 0; i < 100; i++) {
        EXPECT_CMD("t1");
        if (i == 1) {
            EXPECT_CMD("exec2");
        }
    }
    EXPECT_CMD("exec");
}

TEST_F(CdtTest, StartExecuteTaskWithPreTasksSelectPreTaskAndSearchItsOutput) {
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t3");
  EXPECT_CMD("exec3");
  EXPECT_CMD("s\npre pre task");
}

TEST_F(CdtTest, StartExecuteTaskWithPreTasksSelectPreTaskAndOpenLinkFromItsOutput) {
  ProcessExec& exec = mock.cmd_to_process_execs["echo pre pre task 1"].front();
  exec.output_lines = {OUT_LINKS_NOT_HIGHLIGHTED()};
  EXPECT_CDT_STARTED();
  EXPECT_CMD("t3");
  EXPECT_CMD("exec3");
  EXPECT_OUTPUT_LINKS_TO_OPEN();
}
