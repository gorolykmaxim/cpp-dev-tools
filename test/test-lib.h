#ifndef TEST_LIB_H
#define TEST_LIB_H

#include <filesystem>
#include <gmock/gmock-function-mocker.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock-spec-builders.h>
#include <optional>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <map>
#include <entt.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json.hpp>

#include "cdt.h"
#include "process.hpp"

struct Paths {
  using path = std::filesystem::path;
  const path kHome = path("/users") / "my-user";
  const path kUserConfig = kHome / ".cpp-dev-tools.json";
  const path kTasksConfig = kHome / "project" / "tasks.json";
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
  bool fail_to_exec = false;
  bool append_eol = true;
  int exit_code = 0;
};

struct ProcessInfo {
  int exit_code;
  bool is_long = false;
  bool is_finished = false;
  std::function<void()> exit_cb;
  std::string shell_command;
};

class OsApiMock: public OsApi {
public:
  MOCK_METHOD(void, Init, (), (override));
  MOCK_METHOD(std::istream&, In, (), (override));
  MOCK_METHOD(std::ostream&, Out, (), (override));
  MOCK_METHOD(std::ostream&, Err, (), (override));
  MOCK_METHOD(std::string, GetEnv, (const std::string&), (override));
  MOCK_METHOD(void, SetEnv, (const std::string&, const std::string&),
              (override));
  MOCK_METHOD(void, SetCurrentPath, (const std::filesystem::path&),
              (override));
  MOCK_METHOD(std::filesystem::path, GetCurrentPath, (), (override));
  MOCK_METHOD(std::filesystem::path, AbsolutePath,
              (const std::filesystem::path&), (override));
  MOCK_METHOD(bool, ReadFile, (const std::filesystem::path&, std::string&),
              (override));
  MOCK_METHOD(void, WriteFile,
              (const std::filesystem::path&, const std::string&), (override));
  MOCK_METHOD(bool, FileExists, (const std::filesystem::path&), (override));
  MOCK_METHOD(int, Exec, (const std::vector<const char*>&), (override));
  void StartProcess(Process& process,
                    const std::function<void(const char*, size_t)>& stdout_cb,
                    const std::function<void(const char*, size_t)>& stderr_cb,
                    const std::function<void()>& exit_cb) override;
  void FinishProcess(Process& process) override;

  template<typename... Args>
  std::string AssertProcsRanInOrder(Args... args) {
    std::vector<std::string> shell_cmds = {args...};
    if (shell_cmds.empty()) {
      return "";
    }
    int current_cmd = 0;
    std::unordered_set<int> not_finished_cmds;
    for (auto& [_, info]: proc_info) {
      if (shell_cmds[current_cmd] != info.shell_command) {
        continue;
      }
      if (!info.is_finished) {
        not_finished_cmds.insert(current_cmd);
      }
      if (++current_cmd >= shell_cmds.size()) {
        break;
      }
    }
    return GetExpectedVsActualProcsErrorMessage(
        "Expected sequence of process executions not found:",
        current_cmd < shell_cmds.size(), shell_cmds, not_finished_cmds);
  }

  template<typename... Args>
  std::string AssertExactProcsRan(Args... args) {
    std::vector<std::string> shell_cmds = {args...};
    bool equal = shell_cmds.size() == proc_info.size();
    std::unordered_set<int> not_finished_cmds;
    if (equal) {
      int current_cmd = 0;
      for (auto& [_, info]: proc_info) {
        if (shell_cmds[current_cmd] != info.shell_command) {
          equal = false;
          break;
        }
        if (!info.is_finished) {
          not_finished_cmds.insert(current_cmd);
        }
        current_cmd++;
      }
    }
    return GetExpectedVsActualProcsErrorMessage(
        "Expected sequence of process executions to be exactly:", !equal,
        shell_cmds, not_finished_cmds);
  }

  template<typename... Args>
  std::string AssertProcsDidNotRan(Args... args) {
    std::unordered_set<std::string> shell_cmds = {args...};
    if (shell_cmds.empty()) {
      return "";
    }
    std::vector<std::string> unexpected_shell_cmds;
    for (auto& [_, info]: proc_info) {
      if (shell_cmds.count(info.shell_command) > 0) {
        unexpected_shell_cmds.push_back(info.shell_command);
      }
    }
    if (unexpected_shell_cmds.empty()) {
      return "";
    }
    std::stringstream s;
    s << "Unexpected processes executed:\n";
    for (std::string& cmd: unexpected_shell_cmds) {
      s << cmd << '\n';
    }
    s << '\n';
    return s.str();
  }

  int GetProcessExitCode(Process& process) override;
  std::chrono::system_clock::time_point TimeNow() override;
  void PressCtrlC();
  void MockReadFile(const std::filesystem::path& p, const std::string& d);
  void MockReadFile(const std::filesystem::path& p);
  std::string DisplayNotFinishedProcesses();

  using PidType = TinyProcessLib::Process::id_type;
  std::chrono::system_clock::time_point time_now;
  PidType pid_seed = 1;
  std::unordered_map<std::string, std::deque<ProcessExec>> cmd_to_process_execs;
  std::map<PidType, ProcessInfo> proc_info;
  std::unordered_set<PidType> unfinished_procs;
  using ExecProcess = void(const std::string&);
  testing::NiceMock<testing::MockFunction<ExecProcess>> process_calls;

private:
  std::string GetExpectedVsActualProcsErrorMessage(
      const std::string& expected_error_msg, bool not_equal,
      const std::vector<std::string>& shell_cmds,
      const std::unordered_set<int>& not_finished_cmds);
};

#define OUT_LINKS_NOT_HIGHLIGHTED()\
  "/a/b/c:10",\
  "some random data",\
  "/d/e/f:15:32 something",\
  "line /a/b/c:11 and /b/c:32:1"

#define OUT_TEST_ERROR()\
  "unknown file: Failure",\
  "C++ exception with description \"\" thrown in the test body."

#define EXPECT_OUT_EQ_SNAPSHOT(NAME)\
  if (ShouldCreateSnapshot())\
    mock.OsApi::WriteFile(SnapshotPath(NAME), out.str());\
  else\
    EXPECT_EQ(ReadSnapshot(NAME), out.str());\
  out.str("")

#define EXPECT_CDT_STARTED()\
  EXPECT_CALL(mock, Init());\
  EXPECT_TRUE(InitTestCdt());\
  EXPECT_OUT_EQ_SNAPSHOT("InitTestCdt")

#define EXPECT_CDT_STARTED_WITH_PROFILE(PROFILE)\
  mock.MockReadFile(paths.kTasksConfig,\
                    tasks_config_with_profiles_data.dump());\
  EXPECT_CALL(mock, Init());\
  EXPECT_TRUE(InitTestCdt(PROFILE));\
  EXPECT_OUT_EQ_SNAPSHOT("")

#define EXPECT_CDT_ABORTED()\
  EXPECT_FALSE(InitTestCdt());\
  EXPECT_OUT_EQ_SNAPSHOT("")

#define EXPECT_CDT_ABORTED_WITH_PROFILE(PROFILE)\
  mock.MockReadFile(paths.kTasksConfig,\
                    tasks_config_with_profiles_data.dump());\
  EXPECT_FALSE(InitTestCdt(PROFILE));\
  EXPECT_OUT_EQ_SNAPSHOT("")

#define EXPECT_CMD(CMD)\
  RunCmd(CMD);\
  EXPECT_TRUE(mock.unfinished_procs.empty())\
      << mock.DisplayNotFinishedProcesses();\
  EXPECT_OUT_EQ_SNAPSHOT("")

#define EXPECT_INTERRUPTED_CMD(CMD)\
  RunCmd(CMD, true);\
  mock.PressCtrlC();\
  ExecCdtSystems(cdt);\
  EXPECT_TRUE(mock.unfinished_procs.empty())\
      << mock.DisplayNotFinishedProcesses();\
  EXPECT_OUT_EQ_SNAPSHOT("")

#define EXPECT_PROC(CMD)\
  mock.cmd_to_process_execs[CMD].push_back(ProcessExec{});\
  EXPECT_CALL(mock.process_calls, Call(CMD))

#define EXPECT_OUTPUT_LINKS_TO_OPEN()\
  testing::InSequence seq;\
  EXPECT_PROC(execs.kEditor + " /a/b/c:10");\
  EXPECT_PROC(execs.kEditor + " /d/e/f:15:32");\
  EXPECT_PROC(execs.kEditor + " /a/b/c:11");\
  EXPECT_PROC(execs.kEditor + " /b/c:32:1");\
  RunCmd("o1");\
  RunCmd("o2");\
  RunCmd("o3");\
  RunCmd("o4")

#define EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS()\
  EXPECT_CALL(mock.process_calls, Call(testing::_)).Times(0);\
  EXPECT_CMD("o0");\
  EXPECT_CMD("o99");\
  EXPECT_CMD("o")

#define WITH_DEBUG(CMD)\
  "terminal cd " + paths.kTasksConfig.parent_path().string() + " && lldb " + CMD

#define WITH_GT_FILTER(VALUE) " --gtest_filter=\"" VALUE "\""

// Following macros return string instead of const char to silence
// "suspicious string literal probably missing comma" warnings
#define RUNNING_TASK(NAME) std::string("Running \"" NAME "\"")

#define RUNNING_PRE_TASK(NAME) RUNNING_TASK(NAME) + "..."

#define TASK_COMPLETE(NAME) std::string("'" NAME "' complete: return code: 0")

#define TASK_FAILED(NAME, CODE)\
  std::string("'" NAME "' failed: return code: ") + std::to_string(CODE)

#define ASSERT_CDT_STARTED()\
  ASSERT_TRUE(InitTestCdt()) << out.str()

#define ASSERT_CDT_STARTED_WITH_PROFILE(PROFILE)\
  mock.MockReadFile(paths.kTasksConfig,\
                    tasks_config_with_profiles_data.dump());\
  ASSERT_TRUE(InitTestCdt(PROFILE)) << out.str()

#define EXPECT_OUT(...)\
  EXPECT_THAT(out.str(), testing::AllOf(__VA_ARGS__));\
  out.str("")

#define EXPECT_CMDOUT(CMD, ...)\
  RunCmd(CMD);\
  EXPECT_OUT(__VA_ARGS__)

#define EXPECT_INTERRUPTED_CMDOUT(CMD, ...)\
  RunCmd(CMD, true);\
  mock.PressCtrlC();\
  ExecCdtSystems(cdt);\
  EXPECT_OUT(__VA_ARGS__)

#define EXPECT_PROCS(...)\
  if (std::string e = mock.AssertProcsRanInOrder(__VA_ARGS__); !e.empty()) {\
    ADD_FAILURE() << e;\
  }

#define EXPECT_PROCS_EXACT(...)\
  if (std::string e = mock.AssertExactProcsRan(__VA_ARGS__); !e.empty()) {\
    ADD_FAILURE() << e;\
  }

#define EXPECT_NOT_PROCS(...)\
  if (std::string e = mock.AssertProcsDidNotRan(__VA_ARGS__); !e.empty()) {\
    ADD_FAILURE() << e;\
  }

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

MATCHER_P(HasPath, path, "") {
  return arg.find(path.string()) != std::string::npos;
}

MATCHER_P(HasSubstrs, substrs, "") {
  for (const std::string& substr: substrs) {
    if (arg.find(substr) == std::string::npos) {
      return false;
    }
  }
  return true;
}

MATCHER_P(HasSubstrsInOrder, substrs, "") {
  std::string::size_type pos = 0;
  for (const std::string& substr: substrs) {
    std::string::size_type i = arg.find(substr, pos);
    if (i == std::string::npos) {
      return false;
    }
    pos = i + substr.size();
  }
  return true;
}

class CdtTest: public testing::Test {
public:
  Paths paths;
  Executables execs;
  nlohmann::json tasks_config_data, tasks_config_with_profiles_data;
  std::vector<std::string> list_of_tasks_in_ui;
  std::string profile1, profile2;
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
  testing::NiceMock<OsApiMock> mock;
  Cdt cdt;

  static void SetUpTestSuite();
  static bool ShouldCreateSnapshot();
  void SetUp() override;
  bool InitTestCdt(const std::optional<std::string>& profile_name = {});
  nlohmann::json CreateTask(const nlohmann::json& name = nullptr,
                            const nlohmann::json& command = nullptr,
                            const nlohmann::json& pre_tasks = nullptr);
  nlohmann::json CreateTaskAndProcess(const std::string& name,
                                      std::vector<std::string> pre_tasks = {});
  nlohmann::json CreateProfileTaskAndProcess(
      const std::string& name, const std::vector<std::string>& profile_versions,
      std::vector<std::string> pre_tasks = {});
  void RunCmd(const std::string& cmd,
              bool break_when_process_events_stop = false);
  std::filesystem::path SnapshotPath(std::string name);
  std::string ReadSnapshot(const std::string& name);
};

#endif
