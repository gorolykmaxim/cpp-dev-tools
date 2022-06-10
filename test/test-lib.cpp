#include "test-lib.h"
#include "cdt.h"
#include "json.hpp"
#include <atomic>
#include <chrono>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

void OsApiMock::StartProcess(
    Process &process,
    const std::function<void (const char *, size_t)> &stdout_cb,
    const std::function<void (const char *, size_t)> &stderr_cb,
    const std::function<void ()> &exit_cb) {
  auto it = cmd_to_process_execs.find(process.shell_command);
  if (it == cmd_to_process_execs.end()) {
    throw std::runtime_error("Unexpected command called: " +
                             process.shell_command);
  }
  process_calls.Call(process.shell_command);
  std::deque<ProcessExec>& execs = it->second;
  ProcessExec exec = execs.front();
  if (execs.size() > 1) {
    execs.pop_front();
  }
  process.handle = std::unique_ptr<TinyProcessLib::Process>();
  process.id = exec.fail_to_exec ? 0 : pid_seed++;
  unfinished_procs.insert(process.id);
  ProcessInfo& info = proc_info[process.id];
  info.exit_code = exec.exit_code;
  info.exit_cb = exit_cb;
  info.is_long = exec.is_long;
  info.shell_command = process.shell_command;
  if (exec.fail_to_exec) {
    return;
  }
  for (int i = 0; i < exec.output_lines.size(); i++) {
    std::string line = exec.output_lines[i];
    if (exec.append_eol) {
      line += kEol;
    }
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

void OsApiMock::FinishProcess(Process &process) {
  unfinished_procs.erase(process.id);
  proc_info[process.id].is_finished = true;
}

std::string OsApiMock::DisplayNotFinishedProcesses() {
  std::stringstream s;
  s << "Proceses not finished with FinishProcess():\n";
  for (PidType pid: unfinished_procs) {
    s << proc_info[pid].shell_command << '\n';
  }
  return s.str();
}

std::string OsApiMock::GetExpectedVsActualProcsErrorMessage(
    const std::string& expected_error_msg, bool not_equal,
    const std::vector<std::string>& shell_cmds,
    const std::unordered_set<int>& not_finished_cmds) {
  std::stringstream msg;
  if (not_equal) {
    msg << expected_error_msg << '\n';
    for (const std::string& cmd: shell_cmds) {
      msg << cmd << '\n';
    }
    msg << "\nAll actual procsses executed:\n";
    for (auto& [_, info]: proc_info) {
      msg << info.shell_command;
      if (!info.is_finished) {
        msg << " (not finished)";
      }
      msg << '\n';
    }
  } else if (!not_finished_cmds.empty()) {
    msg << "Not all of the specified processes have been finished:\n";
    for (int i = 0; i < shell_cmds.size(); i++) {
      msg << shell_cmds[i];
      if (not_finished_cmds.count(i) > 0) {
        msg << " (not finished)";
      }
      msg << '\n';
    }
  }
  return msg.str();
}

int OsApiMock::GetProcessExitCode(Process &process) {
  return proc_info.at(process.id).exit_code;
}

std::chrono::system_clock::time_point OsApiMock::TimeNow() {
  return time_now += std::chrono::seconds(1);
}

void OsApiMock::PressCtrlC() {
  for (auto& [_, exit_info]: proc_info) {
    if (!exit_info.is_long || exit_info.exit_code == -1) {
      continue;
    }
    exit_info.exit_cb();
    exit_info.exit_code = -1;
  }
}

void OsApiMock::MockReadFile(const std::filesystem::path& p,
                             const std::string& d) {
  EXPECT_CALL(*this, ReadFile(testing::Eq(p), testing::_))
      .WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(d),
                                     testing::Return(true)));
}

void OsApiMock::MockReadFile(const std::filesystem::path& p) {
  EXPECT_CALL(*this, ReadFile(testing::Eq(p), testing::_))
      .WillRepeatedly(testing::Return(false));
}

bool CdtTest::ShouldCreateSnapshot() {
  return std::getenv("SNAPSHOT") != nullptr;
}

void CdtTest::SetUpTestSuite() {
  static bool prev_snapshots_removed = false;
  if (ShouldCreateSnapshot() && !prev_snapshots_removed) {
    std::filesystem::directory_iterator iter(TEST_DATA_DIR);
    for (const std::filesystem::directory_entry& entry: iter) {
      std::filesystem::remove_all(entry.path());
    }
    prev_snapshots_removed = true;
  }
}

void CdtTest::SetUp() {
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
      .WillRepeatedly(testing::Return(paths.kHome.string()));
  EXPECT_CALL(mock, GetEnv(kEnvVarLastCommand))
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::Return(""));
  EXPECT_CALL(mock, GetCurrentPath())
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::Return(paths.kTasksConfig.parent_path()));
  EXPECT_CALL(mock, AbsolutePath(paths.kTasksConfig.filename()))
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::Return(paths.kTasksConfig));
  EXPECT_CALL(mock.process_calls, Call(testing::_))
      .Times(testing::AnyNumber());
  // setup profiles
  profile1 = "profile 1";
  profile2 = "profile 2";
  // mock tasks config
  nlohmann::json tasks_list;
  tasks.helloWorld = CreateTaskAndProcess(tasks_list, "hello world!");
  tasks.primaryTask = CreateTaskAndProcess(tasks_list, "primary task",
                                           {"pre task 1", "pre task 2"});
  tasks.preTask1 = CreateTaskAndProcess(tasks_list, "pre task 1",
                                        {"pre pre task 1", "pre pre task 2"});
  tasks.preTask2 = CreateTaskAndProcess(tasks_list, "pre task 2");
  tasks.prePreTask1 = CreateTaskAndProcess(tasks_list, "pre pre task 1");
  tasks.prePreTask2 = CreateTaskAndProcess(tasks_list, "pre pre task 2");
  tasks.restart = CreateTask(tasks_list, "restart", "__restart");
  tasks.runTests = CreateTask(tasks_list, "run tests",
                              "__gtest " + execs.kTests);
  tasks.taskWithGtestPreTask = CreateTaskAndProcess(tasks_list,
                                                    "task with gtest pre task",
                                                    {"run tests"});
  tasks.runTestsWithPreTasks = CreateTask(tasks_list,
                                          "run tests with pre tasks",
                                          "__gtest " + execs.kTests,
                                          {"pre pre task 1", "pre pre task 2"});
  tasks.buildForPlatformWithProfile = CreateProfileTaskAndProcess(
      tasks_list, "build for {platform} with profile {name}",
      {{profile1, "build for macos with profile profile 1"},
       {profile2, "build for windows with profile profile 2"}});
  tasks.runOnPlatform = CreateProfileTaskAndProcess(
      tasks_list, "run on {platform}",
      {{profile1, "run on macos"}, {profile2, "run on windows"}},
      {"build for {platform} with profile {name}"});
  tasks_config_data["cdt_tasks"] = tasks_list;
  mock.MockReadFile(paths.kTasksConfig, tasks_config_data.dump());
  for (int i = 0; i < tasks_list.size(); i++) {
    std::string index = std::to_string(i + 1);
    std::string name = tasks_list[i]["name"].get<std::string>();
    list_of_tasks_in_ui.push_back(index + " \"" + name + '"');
  }
  // mock tasks config version with profiles in it
  tasks_config_with_profiles_data = tasks_config_data;
  std::vector<nlohmann::json> profiles = {
    {
      {"name", profile1},
      {"platform", "macos"},
    },
    {
      {"name", profile2},
      {"platform", "windows"},
    },
  };
  tasks_config_with_profiles_data["cdt_profiles"] = profiles;
  // mock user config
  nlohmann::json user_config_data;
  user_config_data["open_in_editor_command"] = execs.kEditor + " {}";
  user_config_data["debug_command"] = execs.kNewTerminalTab +
                                      " cd {current_dir} && " +
                                      execs.kDebugger + " {shell_cmd}";
  mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
  EXPECT_CALL(mock, FileExists(paths.kUserConfig))
      .WillRepeatedly(testing::Return(true));
  // mock default test execution
  successful_gtest_exec.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "[==========] Running 3 tests from 2 test suites.",
    "[----------] Global test environment set-up.",
    "[----------] 2 tests from test_suit_1",
    "[ RUN      ] test_suit_1.test1",
    OUT_LINKS_NOT_HIGHLIGHTED(),
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
    "[  PASSED  ] 3 tests.",
  };
  aborted_gtest_exec.exit_code = 1;
  aborted_gtest_exec.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "[==========] Running 2 tests from 2 test suites.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from normal_tests",
    "[ RUN      ] normal_tests.hello_world",
    "[       OK ] normal_tests.hello_world (0 ms)",
    "[----------] 1 test from normal_tests (0 ms total)",
    "",
    "[----------] 1 test from exit_tests",
    "[ RUN      ] exit_tests.exit_in_the_middle",
    OUT_TEST_ERROR(),
  };
  failed_gtest_exec.exit_code = 1;
  failed_gtest_exec.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "[==========] Running 3 tests from 2 test suites.",
    "[----------] Global test environment set-up.",
    "[----------] 2 tests from failed_test_suit_1",
    "[ RUN      ] failed_test_suit_1.test1",
    OUT_LINKS_NOT_HIGHLIGHTED(),
    OUT_TEST_ERROR(),
    "[  FAILED  ] failed_test_suit_1.test1 (0 ms)",
    "[ RUN      ] failed_test_suit_1.test2",
    "[       OK ] failed_test_suit_1.test2 (0 ms)",
    "[----------] 2 tests from failed_test_suit_1 (0 ms total)",
    "",
    "[----------] 1 test from failed_test_suit_2",
    "[ RUN      ] failed_test_suit_2.test1",
    "",
    OUT_TEST_ERROR(),
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
    "2 FAILED TESTS",
  };
  successful_single_gtest_exec.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "Note: Google Test filter = test_suit_1.test1",
    "[==========] Running 1 test from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from test_suit_1",
    "[ RUN      ] test_suit_1.test1",
    OUT_LINKS_NOT_HIGHLIGHTED(),
    "[       OK ] test_suit_1.test1 (0 ms)",
    "[----------] 1 test from test_suit_1 (0 ms total)",
    "",
    "[----------] Global test environment tear-down",
    "[==========] 1 test from 1 test suite ran. (0 ms total)",
    "[  PASSED  ] 1 test.",
  };
  failed_single_gtest_exec.exit_code = 1;
  failed_single_gtest_exec.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "[==========] Running 2 tests from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 2 tests from failed_test_suit_1",
    "[ RUN      ] failed_test_suit_1.test1",
    OUT_LINKS_NOT_HIGHLIGHTED(),
    OUT_TEST_ERROR(),
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
    " 1 FAILED TEST",
  };
  successful_rerun_gtest_exec.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "Note: Google Test filter = failed_test_suit_1.test1",
    "[==========] Running 1 test from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from failed_test_suit_1",
    "[ RUN      ] failed_test_suit_1.test1",
    OUT_LINKS_NOT_HIGHLIGHTED(),
    "[       OK ] failed_test_suit_1.test1 (0 ms)",
    "[----------] 1 test from failed_test_suit_1 (0 ms total)",
    "",
    "[----------] Global test environment tear-down",
    "[==========] 1 test from 1 test suite ran. (0 ms total)",
    "[  PASSED  ] 1 test.",
  };
  failed_rerun_gtest_exec.exit_code = 1;
  failed_rerun_gtest_exec.output_lines = {
    "Running main() from /lib/gtest_main.cc",
    "Note: Google Test filter = failed_test_suit_1.test1",
    "[==========] Running 1 test from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from failed_test_suit_1",
    "[ RUN      ] failed_test_suit_1.test1",
    OUT_LINKS_NOT_HIGHLIGHTED(),
    OUT_TEST_ERROR(),
    "[  FAILED  ] failed_test_suit_1.test1 (0 ms)",
    "[----------] 1 test from failed_test_suit_1 (0 ms total)",
    "",
    "[----------] Global test environment tear-down",
    "[==========] 1 test from 1 test suite ran. (0 ms total)",
    "[  PASSED  ] 0 tests.",
    "[  FAILED  ] 1 test, listed below:",
    "[  FAILED  ] failed_test_suit_1.test1",
    "",
    " 1 FAILED TEST",
  };
  failed_debug_exec.exit_code = 1;
  failed_debug_exec.output_lines = {"failed to launch debugger"};
  failed_debug_exec.stderr_lines.insert(0);
  mock.cmd_to_process_execs[execs.kTests].push_back(successful_gtest_exec);
  std::string one_test = execs.kTests + WITH_GT_FILTER("test_suit_1.test1");
  mock.cmd_to_process_execs[one_test].push_back(successful_single_gtest_exec);
}

bool CdtTest::InitTestCdt(const std::optional<std::string>& profile_name) {
  std::string tasks_config_filename = paths.kTasksConfig.filename().string();
  std::vector<const char*> argv = {
    execs.kCdt.c_str(),
    tasks_config_filename.c_str()
  };
  if (profile_name) {
    argv.push_back(profile_name->c_str());
  }
  return InitCdt(argv.size(), argv.data(), cdt);
}

Task CdtTest::CreateTask(nlohmann::json& tasks, const nlohmann::json& name,
                         const nlohmann::json& command,
                         const nlohmann::json& pre_tasks) {
  Task task;
  nlohmann::json json;
  if (!name.is_null()) {
    task.name = name;
    json["name"] = name;
  }
  if (!command.is_null()) {
    task.command = command;
    json["command"] = command;
  }
  if (!pre_tasks.is_null()) {
    json["pre_tasks"] = pre_tasks;
  }
  tasks.push_back(json);
  return task;
}

Task CdtTest::CreateTaskAndProcess(nlohmann::json &tasks,
                                   const std::string &name,
                                   std::vector<std::string> pre_tasks) {
  std::string cmd = "echo " + name;
  ProcessExec exec;
  exec.output_lines = {name};
  mock.cmd_to_process_execs[cmd].push_back(exec);
  return CreateTask(tasks, name, cmd, std::move(pre_tasks));
}

std::unordered_map<std::string, Task> CdtTest::CreateProfileTaskAndProcess(
    nlohmann::json &tasks, const std::string &name,
    const std::unordered_map<std::string, std::string> &profile_versions,
    std::vector<std::string> pre_tasks) {
  std::unordered_map<std::string, Task> result;
  for (const auto& [k, v]: profile_versions) {
    ProcessExec exec;
    exec.output_lines = {v};
    std::string cmd = "echo " + v;
    mock.cmd_to_process_execs[cmd].push_back(exec);
    Task& task = result[k];
    task.name = v;
    task.command = cmd;
  }
  nlohmann::json json;
  json["name"] = name;
  json["command"] = "echo " + name;
  json["pre_tasks"] = pre_tasks;
  tasks.push_back(json);
  return result;
}

void CdtTest::RunCmd(const std::string& cmd,
                     bool break_when_process_events_stop) {
  in << cmd << std::endl;
  while (true) {
    ExecCdtSystems(cdt);
    if (!break_when_process_events_stop && WillWaitForInput(cdt)) {
      break;
    }
    if (break_when_process_events_stop &&
        cdt.proc_event_queue.size_approx() == 0 &&
        cdt.execs_to_run.empty() &&
        cdt.registry.view<Process>().size() == 1) {
      break;
    }
  }
}

std::filesystem::path CdtTest::SnapshotPath(std::string name) {
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

std::string CdtTest::ReadSnapshot(const std::string& name) {
  std::string snapshot;
  mock.OsApi::ReadFile(SnapshotPath(name), snapshot);
  return snapshot;
}
