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

struct DummyTest {
  std::string name;
  std::vector<std::string> output_lines;
  bool is_failed = false;
};

struct DummyTestSuite {
  std::string name;
  std::vector<DummyTest> tests;
};

class OsApiMock: public OsApi {
public:
  MOCK_METHOD(void, Init, (), (override));
  MOCK_METHOD(std::ostream&, Out, (), (override));
  MOCK_METHOD(std::ostream&, Err, (), (override));
  std::string ReadLineFromStdin() override;
  MOCK_METHOD(std::string, GetEnv, (const std::string&), (override));
  MOCK_METHOD(std::filesystem::path, GetHome, (), (override));
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
  std::string AssertProcsLikeDidNotRan(Args... args) {
    std::vector<std::string> shell_cmds = {args...};
    if (shell_cmds.empty()) {
      return "";
    }
    std::stringstream s;
    bool found = false;
    s << "Unexpected processes executed:\n";
    for (auto& [_, info]: proc_info) {
      for (std::string& cmd: shell_cmds) {
        if (info.shell_command.find(cmd) != std::string::npos) {
          s << '"' << info.shell_command << "\" (matched \"" << cmd << "\")\n";
          found = true;
        }
      }
    }
    return found ? s.str() : "";
  }

  int GetProcessExitCode(Process& process) override;
  std::chrono::system_clock::time_point TimeNow() override;
  void PressCtrlC();
  void MockProc(const std::string& cmd,
                const ProcessExec& exec = ProcessExec{});
  void MockReadFile(const std::filesystem::path& p, const std::string& d);
  void MockReadFile(const std::filesystem::path& p);

  using PidType = TinyProcessLib::Process::id_type;
  std::chrono::system_clock::time_point time_now;
  PidType pid_seed = 1;
  std::unordered_map<std::string, std::deque<ProcessExec>> cmd_to_process_execs;
  std::map<PidType, ProcessInfo> proc_info;
  std::stringstream dummy_stdin;

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

#define OUT_LINKS_HIGHLIGHTED()\
  "\x1B[35m[o1] /a/b/c:10\x1B[0m\n"\
  "some random data\n"\
  "\x1B[35m[o2] /d/e/f:15:32\x1B[0m something\n"\
  "line \x1B[35m[o3] /a/b/c:11\x1B[0m and \x1B[35m[o4] /b/c:32:1\x1B[0m\n"

#define OUT_TEST_ERROR()\
  "unknown file: Failure",\
  "C++ exception with description \"\" thrown in the test body."

#define WITH_DEBUG(CMD)\
  "terminal cd " + paths.kTasksConfig.parent_path().string() + " && lldb " + CMD

#define WITH_GT_FILTER(VALUE) " --gtest_filter=\"" VALUE "\""

// Following macros return string instead of const char to silence
// "suspicious string literal probably missing comma" warnings
#define TASK_COMPLETE(NAME) std::string("'" NAME "' complete: return code: 0")

#define TASK_FAILED(NAME, CODE)\
  std::string("'" NAME "' failed: return code: ") + std::to_string(CODE)

#define ASSERT_INIT_CDT() ASSERT_TRUE(TestCdt()) << current_out_segment

#define ASSERT_CDT_STARTED()\
  ASSERT_TRUE(InitTestCdt()) << out.str();\
  SaveOutput()

#define ASSERT_CDT_STARTED_WITH_PROFILE(PROFILE)\
  mock.MockReadFile(paths.kTasksConfig,\
                    tasks_config_with_profiles_data.dump());\
  ASSERT_TRUE(InitTestCdt(PROFILE)) << out.str();\
  SaveOutput()

#define EXPECT_CDT_FAILED()\
  EXPECT_FALSE(InitTestCdt());\
  SaveOutput()

#define EXPECT_CDT_FAILED_WITH_PROFILE(PROFILE)\
  mock.MockReadFile(paths.kTasksConfig,\
                    tasks_config_with_profiles_data.dump());\
  EXPECT_FALSE(InitTestCdt(PROFILE));\
  SaveOutput()

#define EXPECT_OUT(MATCHER) EXPECT_THAT(current_out_segment, MATCHER)

#define CMD(CMD) RunCmd(CMD)

#define INTERRUPT_CMD(CMD)\
  RunCmd(CMD, true);\
  mock.PressCtrlC();\
  ExecCdtSystems(cdt);\
  SaveOutput()

#define EXPECT_PROCS(...)\
  if (std::string e = mock.AssertProcsRanInOrder(__VA_ARGS__); !e.empty()) {\
    ADD_FAILURE() << e;\
  }

#define EXPECT_PROCS_EXACT(...)\
  if (std::string e = mock.AssertExactProcsRan(__VA_ARGS__); !e.empty()) {\
    ADD_FAILURE() << e;\
  }

#define EXPECT_NOT_PROCS_LIKE(...)\
  if (std::string e = mock.AssertProcsLikeDidNotRan(__VA_ARGS__); !e.empty()) {\
    ADD_FAILURE() << e;\
  }

#define EXPECT_OUTPUT_LINKS_TO_OPEN()\
  mock.cmd_to_process_execs[execs.kEditor + " /a/b/c:10"].push_back(\
      ProcessExec{});\
  mock.cmd_to_process_execs[execs.kEditor + " /d/e/f:15:32"].push_back(\
      ProcessExec{});\
  mock.cmd_to_process_execs[execs.kEditor + " /a/b/c:11"].push_back(\
      ProcessExec{});\
  mock.cmd_to_process_execs[execs.kEditor + " /b/c:32:1"].push_back(\
      ProcessExec{});\
  CMD("o1");\
  CMD("o2");\
  CMD("o3");\
  CMD("o4");\
  EXPECT_PROCS(execs.kEditor + " /a/b/c:10", execs.kEditor + " /d/e/f:15:32",\
               execs.kEditor + " /a/b/c:11", execs.kEditor + " /b/c:32:1")

#define EXPECT_LAST_EXEC_OUTPUT_DISPLAYED_ON_LINK_INDEX_OUT_OF_BOUNDS()\
  CMD("o0");\
  CMD("o99");\
  CMD("o");\
  EXPECT_NOT_PROCS_LIKE(execs.kEditor)

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

MATCHER_P2(HasSubstrNTimes, substr, times, "") {
  std::string s(substr);
  int cnt = 0;
  std::string::size_type pos = 0;
  while (true) {
    std::string::size_type i = arg.find(s, pos);
    pos = i + s.size();
    if (i == std::string::npos) {
      break;
    }
    cnt++;
  }
  return cnt == times;
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
  nlohmann::json tasks_config_data, tasks_config_with_profiles_data,
                 user_config_data;
  std::vector<nlohmann::json> tasks;
  std::vector<nlohmann::json> profiles;
  std::vector<std::string> args;
  std::vector<std::string> list_of_tasks_in_ui;
  std::vector<std::string> test_list_successful;
  std::vector<std::string> test_list_failed;
  std::string profile1, profile2;
  ProcessExec successful_gtest_exec,
              failed_gtest_exec,
              successful_single_gtest_exec,
              failed_single_gtest_exec,
              aborted_gtest_exec,
              successful_rerun_gtest_exec,
              failed_rerun_gtest_exec;
  std::stringstream out;
  std::string current_out_segment;
  testing::NiceMock<OsApiMock> mock;
  Cdt cdt;

  void Init();
  void SetUp() override;
  bool TestCdt();
  bool InitTestCdt(const std::optional<std::string>& profile_name = {});
  nlohmann::json CreateTask(const nlohmann::json& name = nullptr,
                            const nlohmann::json& command = nullptr,
                            const nlohmann::json& pre_tasks = nullptr);
  nlohmann::json CreateTaskAndProcess(const std::string& name,
                                      std::vector<std::string> pre_tasks = {});
  nlohmann::json CreateProfileTaskAndProcess(
      const std::string& name, const std::vector<std::string>& profile_versions,
      std::vector<std::string> pre_tasks = {});
  std::vector<std::string> CreateTestOutput(
      const std::vector<DummyTestSuite>& suites);
  std::vector<std::string> CreateAbortedTestOutput(const std::string& suite,
                                                   const std::string& test);
  void RunCmd(const std::string& cmd,
              bool break_when_process_events_stop = false);
  void InterruptCmd(const std::string& cmd);
  void SaveOutput();
};

#endif
