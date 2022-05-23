#include <cstddef>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "cdt.h"
#include "gtest.h"
#include "common.h"

static std::string kGtest;
static std::string kGtestSearch;
static std::string kGtestRerun;
static std::string kGtestRerunRepeat;
static std::string kGtestDebug;
static std::string kGtestFilter;

void InitGtest(Cdt& cdt) {
    kGtest = DefineUserCommand("g", {"ind", "Display output of the specified google test"}, cdt);
    kGtestSearch = DefineUserCommand("gs", {"ind", "Search through output of the specified google test with the specified regular expression"}, cdt);
    kGtestRerun = DefineUserCommand("gt", {"ind", "Re-run the google test with the specified index"}, cdt);
    kGtestRerunRepeat = DefineUserCommand("gtr", {"ind", "Keep re-running the google test with the specified index until it fails"}, cdt);
    kGtestDebug = DefineUserCommand("gd", {"ind", "Re-run the google test with the specified index with debugger attached"}, cdt);
    kGtestFilter = DefineUserCommand("gf", {"ind", "Run google tests of the task with the specified index with a specified " + kGtestFilterArg}, cdt);
}

static std::string GtestTaskCommandToShellCommand(const std::string& task_cmd, const std::optional<std::string>& gtest_filter = {}) {
    std::string binary = task_cmd.substr(kGtestTask.size() + 1);
    if (gtest_filter) {
        binary += " " + kGtestFilterArg + "='" + *gtest_filter + "'";
    }
    return binary;
}

void ScheduleGtestExecutions(Cdt& cdt) {
  for (entt::entity entity: cdt.execs_to_run) {
    Execution& exec = cdt.registry.get<Execution>(entity);
    Output& output = cdt.registry.get<Output>(entity);
    if (cdt.registry.all_of<GtestExecution>(entity) ||
        exec.shell_command.find(kGtestTask) != 0) {
      continue;
    }
    if (exec.debug == DebugStatus::kNotRequired) {
      GtestExecution& test = cdt.registry.emplace<GtestExecution>(entity);
      test.display_progress = output.mode == OutputMode::kStream;
      output.mode = OutputMode::kSilent;
    }
    exec.is_pinned = true;
    exec.shell_command = GtestTaskCommandToShellCommand(exec.shell_command);
  }
}

static void CompleteCurrentGtest(GtestExecution& gtest_exec, const std::string& line_content, bool& test_completed) {
    gtest_exec.tests[*gtest_exec.current_test].duration = line_content.substr(line_content.rfind('('));
    gtest_exec.current_test.reset();
    test_completed = true;
}

void ParseGtestOutput(Cdt& cdt) {
  static size_t kTestCountIndex = std::string("Running ").size();
  auto view = cdt.registry.view<Running, Output, GtestExecution>();
  for (auto [_, output, gtest_exec]: view.each()) {
    if (gtest_exec.state != GtestExecutionState::kRunning &&
        gtest_exec.state != GtestExecutionState::kParsing) {
      continue;
    }
    for (int l = gtest_exec.lines_parsed; l < output.lines.size(); l++) {
      std::string& line = output.lines[l];
      int line_content_index = 0;
      char filler_char = '\0';
      std::stringstream word_stream;
      for (int i = 0; i < line.size(); i++) {
        if (i == 0) {
          if (line[i] == '[') {
            continue;
          } else {
            break;
          }
        }
        if (i == 1) {
          filler_char = line[i];
          continue;
        }
        if (line[i] == ']') {
          line_content_index = i + 2;
          break;
        }
        if (line[i] != filler_char) {
          word_stream << line[i];
        }
      }
      std::string found_word = word_stream.str();
      std::string line_content = line.substr(line_content_index);
      bool test_completed = false;
      /*
      The order of conditions here is important:
      We might be executing google tests, that execute their own google tests
      as a part of their routine. We must differentiate between
      the output of tests launched by us and any other output
      that might look like google test output.
      As a first priority - we handle the output as a part of output
      of the current test. When getting "OK" and "FAILED" we also check
      that those two actually belong to OUR current test. Only in case
      we are not running a test it is safe to process other cases,
      since they are guaranteed to belong to our tests and not some other
      child tests.
      */
      if (found_word == "OK" &&
          line_content.find(gtest_exec.tests.back().name) == 0) {
        CompleteCurrentGtest(gtest_exec, line_content, test_completed);
      } else if (found_word == "FAILED" &&
                 line_content.find(gtest_exec.tests.back().name) == 0) {
        gtest_exec.failed_test_ids.push_back(*gtest_exec.current_test);
        CompleteCurrentGtest(gtest_exec, line_content, test_completed);
      } else if (gtest_exec.current_test) {
        gtest_exec.tests[*gtest_exec.current_test].buffer_end++;
        if (gtest_exec.rerun_of_single_test) {
          cdt.output.lines.push_back(line);
        }
      } else if (found_word == "RUN") {
        gtest_exec.current_test = gtest_exec.tests.size();
        GtestTest test{line_content};
        test.buffer_start = l + 1;
        test.buffer_end = test.buffer_start;
        gtest_exec.tests.push_back(test);
      } else if (filler_char == '=') {
        if (gtest_exec.state == GtestExecutionState::kRunning) {
          std::string::size_type count_end_index = line_content.find(
              ' ', kTestCountIndex);
          std::string count_str = line_content.substr(
              kTestCountIndex, count_end_index - kTestCountIndex);
          gtest_exec.test_count = std::stoi(count_str);
          gtest_exec.tests.reserve(gtest_exec.test_count);
          gtest_exec.state = GtestExecutionState::kParsing;
        } else {
          std::string::size_type pos = line_content.rfind('(');
          gtest_exec.total_duration = line_content.substr(pos);
          gtest_exec.state = GtestExecutionState::kParsed;
          break;
        }
      }
      if (gtest_exec.display_progress &&
          !gtest_exec.rerun_of_single_test && test_completed) {
        cdt.os->Out() << std::flush << "\rTests completed: "
                      << gtest_exec.tests.size() << " of "
                      << gtest_exec.test_count;
      }
    }
    gtest_exec.lines_parsed = output.lines.size();
  }
}

static void PrintGtestList(const std::vector<int>& test_ids,
                           const std::vector<GtestTest>& tests, Cdt& cdt) {
  for (int i = 0; i < test_ids.size(); i++) {
    int id = test_ids[i];
    const GtestTest& test = tests[id];
    cdt.os->Out() << i + 1 << " \"" << test.name << "\" " << test.duration
                  << std::endl;
  }
}

static void PrintFailedGtestList(const GtestExecution& exec, Cdt& cdt) {
    cdt.os->Out() << kTcRed << "Failed tests:" << kTcReset << std::endl;
    PrintGtestList(exec.failed_test_ids, exec.tests, cdt);
    int failed_percent = exec.failed_test_ids.size() / (float)exec.tests.size() * 100;
    cdt.os->Out() << kTcRed << "Tests failed: " << exec.failed_test_ids.size() << " of " << exec.tests.size() << " (" << failed_percent << "%) " << exec.total_duration << kTcReset << std::endl;
}

static void PrintGtestOutput(ConsoleOutput& console, const Output& output,
                             const GtestTest& test, const std::string& color) {
  console = ConsoleOutput{};
  console.lines.reserve(test.buffer_end - test.buffer_start + 1);
  console.lines.emplace_back(color + '"' + test.name + "\" output:" +
                             kTcReset);
  console.lines.insert(console.lines.end(),
                       output.lines.begin() + test.buffer_start,
                       output.lines.begin() + test.buffer_end);
}

static bool FindGtestByCmdArgInLastEntityWithGtestExec(const UserCommand& cmd,
                                                       Cdt& cdt,
                                                       entt::entity& entity,
                                                       GtestExecution& exec,
                                                       GtestTest& test) {
  size_t lookup_start = 0;
  if (cdt.selected_exec) {
    auto it = std::find(cdt.exec_history.begin(), cdt.exec_history.end(),
                        *cdt.selected_exec);
    if (it != cdt.exec_history.end()) {
      lookup_start = it - cdt.exec_history.begin();
    }
  }
  for (size_t i = lookup_start; i < cdt.exec_history.size(); i++) {
    entity = cdt.exec_history[i];
    if (cdt.registry.all_of<GtestExecution>(entity)) {
      exec = cdt.registry.get<GtestExecution>(entity);
      break;
    }
  }
  if (exec.test_count == 0) {
    cdt.os->Out() << kTcGreen << "No google tests have been executed yet."
                  << kTcReset << std::endl;
  } else if (exec.failed_test_ids.empty()) {
    if (!IsCmdArgInRange(cmd, exec.tests)) {
      cdt.os->Out() << kTcGreen << "Last executed tests " << exec.total_duration
                    << ":" << kTcReset << std::endl;
      std::vector<int> ids(exec.tests.size());
      std::iota(ids.begin(), ids.end(), 0);
      PrintGtestList(ids, exec.tests, cdt);
    } else {
      test = exec.tests[cmd.arg - 1];
      return true;
    }
  } else {
    if (!IsCmdArgInRange(cmd, exec.failed_test_ids)) {
      PrintFailedGtestList(exec, cdt);
    } else {
      test = exec.tests[exec.failed_test_ids[cmd.arg - 1]];
      return true;
    }
  }
  return false;
}

void DisplayGtestOutput(Cdt& cdt) {
  if (!AcceptUsrCmd(kGtest, cdt.last_usr_cmd)) return;
  entt::entity entity;
  GtestExecution gtest_exec;
  GtestTest test;
  if (!FindGtestByCmdArgInLastEntityWithGtestExec(cdt.last_usr_cmd, cdt,
                                                  entity, gtest_exec,
                                                  test)) {
    return;
  }
  Output& output = cdt.registry.get<Output>(entity);
  std::string color = gtest_exec.failed_test_ids.empty() ? kTcGreen : kTcRed;
  PrintGtestOutput(cdt.output, output, test, color);
}

void SearchThroughGtestOutput(Cdt& cdt) {
  if (!AcceptUsrCmd(kGtestSearch, cdt.last_usr_cmd)) return;
  entt::entity entity;
  GtestExecution gtest_exec;
  GtestTest test;
  if (!FindGtestByCmdArgInLastEntityWithGtestExec(cdt.last_usr_cmd, cdt,
                                                  entity, gtest_exec, test)) {
    return;
  }
  cdt.registry.emplace<OutputSearch>(entity, test.buffer_start,
                                     test.buffer_end);
}

void RerunGtest(Cdt& cdt) {
  if (!AcceptUsrCmd(kGtestRerun, cdt.last_usr_cmd) &&
      !AcceptUsrCmd(kGtestRerunRepeat, cdt.last_usr_cmd) &&
      !AcceptUsrCmd(kGtestDebug, cdt.last_usr_cmd)) {
    return;
  }
  entt::entity entity;
  GtestExecution gtest_exec;
  GtestTest test;
  if (!FindGtestByCmdArgInLastEntityWithGtestExec(cdt.last_usr_cmd, cdt,
                                                  entity, gtest_exec, test)) {
    return;
  }
  size_t task_id = cdt.registry.get<Execution>(entity).task_id;
  entity = cdt.registry.create();
  Execution& exec = cdt.registry.emplace<Execution>(entity);
  exec.name = test.name;
  exec.shell_command = GtestTaskCommandToShellCommand(
      cdt.tasks[task_id].command, test.name);
  exec.task_id = task_id;
  exec.repeat_until_fail = kGtestRerunRepeat == cdt.last_usr_cmd.cmd;
  if (kGtestDebug == cdt.last_usr_cmd.cmd) {
    exec.debug = DebugStatus::kRequired;
  } else {
    cdt.registry.emplace<GtestExecution>(entity, true, true);
  }
  cdt.registry.emplace<Output>(entity, OutputMode::kSilent);
  cdt.registry.emplace<ToSchedule>(entity);
}

void ScheduleGtestTaskWithFilter(Cdt& cdt) {
  if (!AcceptUsrCmd(kGtestFilter, cdt.last_usr_cmd)) return;
  if (!IsCmdArgInRange(cdt.last_usr_cmd, cdt.tasks)) {
    DisplayListOfTasks(cdt.tasks, cdt);
  } else {
    std::string filter = ReadInputFromStdin(kGtestFilterArg + "=", cdt);
    int task_id = cdt.last_usr_cmd.arg - 1;
    entt::entity entity = cdt.registry.create();
    Execution& exec = cdt.registry.emplace<Execution>(entity);
    exec.name = filter;
    exec.shell_command = GtestTaskCommandToShellCommand(
        cdt.tasks[task_id].command, filter);
    exec.is_pinned = true;
    exec.task_id = task_id;
    cdt.registry.emplace<GtestExecution>(entity, true);
    cdt.registry.emplace<Output>(entity, OutputMode::kSilent);
    cdt.registry.emplace<ToSchedule>(entity);
  }
}

void DisplayGtestExecutionResult(Cdt& cdt) {
  auto view = cdt.registry.view<Execution, Process, Output, GtestExecution,
                                Running>();
  for (auto [_, exec, proc, output, gtest_exec]: view.each()) {
    if (gtest_exec.rerun_of_single_test ||
        proc.state == ProcessState::kRunning ||
        gtest_exec.state == GtestExecutionState::kFinished) {
      continue;
    }
    // Current line has test execution progress displayed.
    // We will start displaying our output on top of it.
    cdt.os->Out() << "\33[2K\r";
    if (gtest_exec.state == GtestExecutionState::kRunning) {
      proc.state = ProcessState::kFailed;
      Task& task = cdt.tasks[exec.task_id];
      std::string cmd = GtestTaskCommandToShellCommand(task.command);
      cdt.os->Out() << kTcRed << "'" << cmd
                    << "' is not a google test executable" << kTcReset
                    << std::endl;
    } else if (gtest_exec.state == GtestExecutionState::kParsing) {
      proc.state = ProcessState::kFailed;
      cdt.os->Out() << kTcRed << "Tests have finished prematurely" << kTcReset
                    << std::endl;
      // Tests might have crashed in the middle of some test.
      // If so - consider test failed.
      // This is not always true however: tests might have crashed
      // in between two test cases.
      if (gtest_exec.current_test) {
        gtest_exec.failed_test_ids.push_back(*gtest_exec.current_test);
        gtest_exec.current_test.reset();
      }
    } else if (gtest_exec.failed_test_ids.empty() &&
               gtest_exec.display_progress) {
      cdt.os->Out() << kTcGreen << "Successfully executed "
                    << gtest_exec.tests.size() << " tests "
                    << gtest_exec.total_duration << kTcReset << std::endl;
    }
    if (!gtest_exec.failed_test_ids.empty()) {
      PrintFailedGtestList(gtest_exec, cdt);
      if (gtest_exec.failed_test_ids.size() == 1) {
        GtestTest& test = gtest_exec.tests[gtest_exec.failed_test_ids[0]];
        PrintGtestOutput(cdt.output, output, test, kTcRed);
      }
    }
    gtest_exec.state = GtestExecutionState::kFinished;
  }
}

void RestartRepeatingGtestOnSuccess(Cdt& cdt) {
  auto view = cdt.registry.view<Execution, GtestExecution, Process, Running>();
  for (auto [entity, exec, gtest_exec, proc]: view.each()) {
    if (proc.state == ProcessState::kComplete && exec.repeat_until_fail) {
      GtestExecution new_gtest_exec;
      new_gtest_exec.display_progress = gtest_exec.display_progress;
      new_gtest_exec.rerun_of_single_test = gtest_exec.rerun_of_single_test;
      gtest_exec = new_gtest_exec;
    }
  }
}

void FinishGtestExecution(Cdt& cdt) {
  bool unpin_existing_entities = false;
  std::vector<entt::entity> to_remove;
  auto view = cdt.registry.view<Process, GtestExecution, Running>();
  for (auto [entity, proc, gtest_exec]: view.each()) {
    if (proc.state == ProcessState::kRunning) {
      continue;
    }
    if (gtest_exec.rerun_of_single_test) {
      to_remove.push_back(entity);
    } else {
      unpin_existing_entities = true;
    }
  }
  cdt.registry.erase<GtestExecution>(to_remove.begin(), to_remove.end());
  if (unpin_existing_entities) {
    for (entt::entity entity: cdt.exec_history) {
      if (cdt.registry.all_of<GtestExecution>(entity)) {
        cdt.registry.get<Execution>(entity).is_pinned = false;
      }
    }
  }
}
