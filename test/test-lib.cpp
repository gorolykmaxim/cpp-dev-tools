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
#include <unordered_set>
#include <vector>

std::string OsApiMock::ReadLineFromStdin() {
  std::string line;
  std::getline(dummy_stdin, line);
  return line;
}

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
  std::deque<ProcessExec>& execs = it->second;
  ProcessExec exec = execs.front();
  if (execs.size() > 1) {
    execs.pop_front();
  }
  process.handle = std::unique_ptr<TinyProcessLib::Process>();
  process.id = exec.fail_to_exec ? 0 : pid_seed++;
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
      line += '\n';
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
  proc_info[process.id].is_finished = true;
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

void OsApiMock::MockProc(const std::string &cmd, const ProcessExec& exec) {
  cmd_to_process_execs[cmd].push_back(exec);
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

void CdtTest::SetUp() {
  args = {execs.kCdt, paths.kTasksConfig.filename().string()};
  user_config_data["open_in_editor_command"] = execs.kEditor + " {}";
  user_config_data["debug_command"] = execs.kNewTerminalTab +
                                      " cd {current_dir} && " +
                                      execs.kDebugger + " {shell_cmd}";
  cdt.os = &mock;
  EXPECT_CALL(mock, Out())
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::ReturnRef(out));
  EXPECT_CALL(mock, Err())
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::ReturnRef(out));
  EXPECT_CALL(mock, GetHome())
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::Return(paths.kHome));
  EXPECT_CALL(mock, GetEnv(kEnvVarLastCommand))
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::Return(""));
  EXPECT_CALL(mock, GetCurrentPath())
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::Return(paths.kTasksConfig.parent_path()));
  EXPECT_CALL(mock, AbsolutePath(paths.kTasksConfig.filename()))
      .Times(testing::AnyNumber())
      .WillRepeatedly(testing::Return(paths.kTasksConfig));
  EXPECT_CALL(mock, FileExists(paths.kUserConfig))
      .WillRepeatedly(testing::Return(true));
  mock.MockReadFile(paths.kUserConfig, user_config_data.dump());
  MockTasksConfig();
}

bool CdtTest::InitTestCdt() {
  std::vector<const char*> argv;
  argv.reserve(args.size());
  for (const std::string& arg: args) {
    argv.push_back(arg.c_str());
  }
  bool result = InitCdt(argv.size(), argv.data(), cdt);
  SaveOutput();
  return result;
}

nlohmann::json CdtTest::CreateTask(const nlohmann::json& name,
                                   const nlohmann::json& command,
                                   const nlohmann::json& pre_tasks) {
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

nlohmann::json CdtTest::CreateTaskAndProcess(
    const std::string& name, std::vector<std::string> pre_tasks) {
  std::string cmd = "echo " + name;
  ProcessExec exec;
  exec.output_lines = {name};
  mock.MockProc(cmd, exec);
  return CreateTask(name, cmd, std::move(pre_tasks));
}

std::vector<std::string> CdtTest::CreateTestOutput(
    const std::vector<DummyTestSuite> &suites) {
  int total_test_count = 0;
  std::vector<std::string> failed_tests;
  std::vector<std::string> output(3);
  output[0] = "Running main() from /lib/gtest_main.cc";
  output[2] = "[----------] Global test environment set-up.";
  for (const DummyTestSuite& suite: suites) {
    total_test_count += suite.tests.size();
    std::string test_count = std::to_string(suite.tests.size());
    output.push_back("[----------] " + test_count + " tests from " +
                     suite.name);
    for (const DummyTest& test: suite.tests) {
      std::string test_name = suite.name + "." + test.name;
      output.push_back("[ RUN      ] " + test_name);
      output.insert(output.end(), test.output_lines.begin(),
                    test.output_lines.end());
      if (test.is_failed) {
        output.push_back("[  FAILED  ] " + test_name + " (0 ms)");
        failed_tests.push_back(test_name);
      } else {
        output.push_back("[       OK ] " + test_name + " (0 ms)");
      }
    }
    output.push_back("[----------] " + test_count + " tests from " +
                     suite.name + " (0 ms total)");
    output.push_back("");
  }
  std::string tests_from_test_suites = std::to_string(total_test_count) +
                                       " tests from " +
                                       std::to_string(suites.size()) +
                                       " test suites";
  output[1] = "[==========] Running " + tests_from_test_suites + ".";
  output.push_back("[----------] Global test environment tear-down");
  output.push_back("[==========] " + tests_from_test_suites +
                   " ran. (0 ms total)");
  output.push_back("[  PASSED  ] " +
                   std::to_string(total_test_count - failed_tests.size()) +
                   " tests.");
  if (!failed_tests.empty()) {
    std::string failed_test_count = std::to_string(failed_tests.size());
    output.push_back("[  FAILED  ] " + failed_test_count +
                     " tests, listed below:");
    for (std::string& failed_test: failed_tests) {
      output.push_back("[  FAILED  ] " + failed_test);
    }
    output.push_back("");
    output.push_back(failed_test_count + " FAILED TESTS");
  }
  return output;
}

std::vector<std::string> CdtTest::CreateAbortedTestOutput(
    const std::string &suite, const std::string &test) {
  return {
    "Running main() from /lib/gtest_main.cc",
    "[==========] Running 1 test from 1 test suite.",
    "[----------] Global test environment set-up.",
    "[----------] 1 test from " + suite,
    "[ RUN      ] " + suite + '.' + test,
  };
}

void CdtTest::MockTasksConfig() {
  nlohmann::json tasks_config = {
      {"cdt_tasks", tasks},
      {"cdt_profiles", profiles}};
  mock.MockReadFile(paths.kTasksConfig, tasks_config.dump());
}

void CdtTest::RunCmd(const std::string& cmd,
                     bool break_when_process_events_stop) {
  mock.dummy_stdin << cmd << std::endl;
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
  if (!break_when_process_events_stop) {
    SaveOutput();
  }
}

void CdtTest::InterruptCmd(const std::string &cmd) {
  RunCmd(cmd, true);
  mock.PressCtrlC();
  ExecCdtSystems(cdt);
  SaveOutput();
}

void CdtTest::SaveOutput() {
  current_out_segment = out.str();
  out.str("");
}
