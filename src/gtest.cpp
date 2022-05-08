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
    for (Entity entity: cdt.execs_to_run) {
        Process& proc = cdt.processes[entity];
        if (Find(entity, cdt.gtest_execs) || proc.shell_command.find(kGtestTask) != 0) {
            continue;
        }
        if (proc.debug == DebugStatus::kNotRequired) {
            cdt.gtest_execs[entity] = GtestExecution{};
        }
        Execution& exec = cdt.execs[entity];
        exec.is_pinned = true;
        proc.shell_command = GtestTaskCommandToShellCommand(proc.shell_command);
    }
}

static void CompleteCurrentGtest(GtestExecution& gtest_exec, const std::string& line_content, bool& test_completed) {
    gtest_exec.tests[*gtest_exec.current_test].duration = line_content.substr(line_content.rfind('('));
    gtest_exec.current_test.reset();
    test_completed = true;
}

void ParseGtestOutput(Cdt& cdt) {
    static size_t kTestCountIndex = std::string("Running ").size();
    for (Entity entity: cdt.running_execs) {
        std::vector<std::string>& proc_buffer = cdt.text_buffers[entity][TextBufferType::kProcess];
        std::vector<std::string>& gtest_buffer = cdt.text_buffers[entity][TextBufferType::kGtest];
        std::vector<std::string>& out_buffer = cdt.text_buffers[entity][TextBufferType::kOutput];
        Process& proc = cdt.processes[entity];
        GtestExecution* gtest_exec = Find(entity, cdt.gtest_execs);
        if (!gtest_exec) {
            continue;
        }
        if (gtest_exec->state == GtestExecutionState::kRunning || gtest_exec->state == GtestExecutionState::kParsing) {
            for (std::string& line: proc_buffer) {
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
                We might be executing google tests, that execute their own google tests as a part of their
                routine. We must differentiate between the output of tests launched by us and any other output
                that might look like google test output.
                As a first priority - we handle the output as a part of output of the current test. When getting
                "OK" and "FAILED" we also check that those two actually belong to OUR current test.
                Only in case we are not running a test it is safe to process other cases, since they are guaranteed
                to belong to our tests and not some other child tests.
                */
                if (found_word == "OK" && line_content.find(gtest_exec->tests.back().name) == 0) {
                    CompleteCurrentGtest(*gtest_exec, line_content, test_completed);
                } else if (found_word == "FAILED" && line_content.find(gtest_exec->tests.back().name) == 0) {
                    gtest_exec->failed_test_ids.push_back(*gtest_exec->current_test);
                    CompleteCurrentGtest(*gtest_exec, line_content, test_completed);
                } else if (gtest_exec->current_test) {
                    gtest_exec->tests[*gtest_exec->current_test].buffer_end++;
                    gtest_buffer.push_back(line);
                    if (proc.stream_output && gtest_exec->rerun_of_single_test) {
                        out_buffer.push_back(line);
                    }
                } else if (found_word == "RUN") {
                    gtest_exec->current_test = gtest_exec->tests.size();
                    GtestTest test{line_content};
                    test.buffer_start = gtest_buffer.size();
                    test.buffer_end = test.buffer_start;
                    gtest_exec->tests.push_back(test);
                } else if (filler_char == '=') {
                    if (gtest_exec->state == GtestExecutionState::kRunning) {
                        std::string::size_type count_end_index = line_content.find(' ', kTestCountIndex);
                        std::string count_str = line_content.substr(kTestCountIndex, count_end_index - kTestCountIndex);
                        gtest_exec->test_count = std::stoi(count_str);
                        gtest_exec->tests.reserve(gtest_exec->test_count);
                        gtest_exec->state = GtestExecutionState::kParsing;
                    } else {
                        gtest_exec->total_duration = line_content.substr(line_content.rfind('('));
                        gtest_exec->state = GtestExecutionState::kParsed;
                        break;
                    }
                }
                if (proc.stream_output && !gtest_exec->rerun_of_single_test && test_completed) {
                    cdt.os->Out() << std::flush << "\rTests completed: " << gtest_exec->tests.size() << " of " << gtest_exec->test_count;
                }
            }
        }
        proc_buffer.clear();
    }
}

static void PrintGtestList(const std::vector<size_t>& test_ids, const std::vector<GtestTest>& tests, Cdt& cdt) {
    for (int i = 0; i < test_ids.size(); i++) {
        size_t id = test_ids[i];
        const GtestTest& test = tests[id];
        cdt.os->Out() << i + 1 << " \"" << test.name << "\" " << test.duration << std::endl;
    }
}

static void PrintFailedGtestList(const GtestExecution& exec, Cdt& cdt) {
    cdt.os->Out() << kTcRed << "Failed tests:" << kTcReset << std::endl;
    PrintGtestList(exec.failed_test_ids, exec.tests, cdt);
    int failed_percent = exec.failed_test_ids.size() / (float)exec.tests.size() * 100;
    cdt.os->Out() << kTcRed << "Tests failed: " << exec.failed_test_ids.size() << " of " << exec.tests.size() << " (" << failed_percent << "%) " << exec.total_duration << kTcReset << std::endl;
}

static void PrintGtestOutput(ExecutionOutput& out, const std::vector<std::string>& gtest_buffer, std::vector<std::string>& out_buffer, const GtestTest& test, const std::string& color) {
    out = ExecutionOutput{};
    out_buffer.clear();
    out_buffer.reserve(test.buffer_end - test.buffer_start + 1);
    out_buffer.emplace_back(color + "\"" + test.name + "\" output:" + kTcReset);
    out_buffer.insert(out_buffer.end(), gtest_buffer.begin() + test.buffer_start, gtest_buffer.begin() + test.buffer_end);
}

static bool FindGtestByCmdArgInLastEntityWithGtestExec(const UserCommand& cmd, Cdt& cdt, Entity& entity, GtestExecution& exec, GtestTest& test) {
    size_t lookup_start = 0;
    if (cdt.selected_exec) {
        auto it = std::find(cdt.exec_history.begin(), cdt.exec_history.end(), *cdt.selected_exec);
        if (it != cdt.exec_history.end()) {
            lookup_start = it - cdt.exec_history.begin();
        }
    }
    for (size_t i = lookup_start; i < cdt.exec_history.size(); i++) {
        entity = cdt.exec_history[i];
        auto it = cdt.gtest_execs.find(entity);
        if (it != cdt.gtest_execs.end()) {
            exec = it->second;
            break;
        }
    }
    if (exec.test_count == 0) {
        cdt.os->Out() << kTcGreen << "No google tests have been executed yet." << kTcReset << std::endl;
    } else if (exec.failed_test_ids.empty()) {
        if (!IsCmdArgInRange(cmd, exec.tests)) {
            cdt.os->Out() << kTcGreen << "Last executed tests " << exec.total_duration << ":" << kTcReset << std::endl;
            std::vector<size_t> ids(exec.tests.size());
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
    Entity entity;
    GtestExecution gtest_exec;
    GtestTest test;
    if (!FindGtestByCmdArgInLastEntityWithGtestExec(cdt.last_usr_cmd, cdt, entity, gtest_exec, test)) {
        return;
    }
    ExecutionOutput& exec_output = cdt.exec_outputs[entity];
    std::vector<std::string>& gtest_buffer = cdt.text_buffers[entity][TextBufferType::kGtest];
    std::vector<std::string>& out_buffer = cdt.text_buffers[entity][TextBufferType::kOutput];
    PrintGtestOutput(exec_output, gtest_buffer, out_buffer, test, gtest_exec.failed_test_ids.empty() ? kTcGreen : kTcRed);
}

void SearchThroughGtestOutput(Cdt& cdt) {
    if (!AcceptUsrCmd(kGtestSearch, cdt.last_usr_cmd)) return;
    Entity entity;
    GtestExecution gtest_exec;
    GtestTest test;
    if (!FindGtestByCmdArgInLastEntityWithGtestExec(cdt.last_usr_cmd, cdt, entity, gtest_exec, test)) {
        return;
    }
    cdt.text_buffer_searchs[entity] = TextBufferSearch{TextBufferType::kGtest, test.buffer_start, test.buffer_end};
}

void RerunGtest(Cdt& cdt) {
    if (!AcceptUsrCmd(kGtestRerun, cdt.last_usr_cmd) && !AcceptUsrCmd(kGtestRerunRepeat, cdt.last_usr_cmd) && !AcceptUsrCmd(kGtestDebug, cdt.last_usr_cmd)) return;
    Entity entity;
    GtestExecution gtest_exec;
    GtestTest test;
    if (!FindGtestByCmdArgInLastEntityWithGtestExec(cdt.last_usr_cmd, cdt, entity, gtest_exec, test)) {
        return;
    }
    size_t task_id = cdt.execs[entity].task_id;
    entity = CreateEntity(cdt);
    Execution& exec = cdt.execs[entity];
    exec.name = test.name;
    exec.task_id = task_id;
    exec.repeat_until_fail = kGtestRerunRepeat == cdt.last_usr_cmd.cmd;
    Process& proc = cdt.processes[entity];
    proc.shell_command = GtestTaskCommandToShellCommand(cdt.tasks[task_id].command, test.name);
    proc.stream_output = true;
    if (kGtestDebug == cdt.last_usr_cmd.cmd) {
        proc.debug = DebugStatus::kRequired;
    } else {
        GtestExecution& new_gtest_exec = cdt.gtest_execs[entity];
        new_gtest_exec.rerun_of_single_test = true;
    }
    cdt.execs_to_schedule.push_back(entity);
}

void ScheduleGtestTaskWithFilter(Cdt& cdt) {
    if (!AcceptUsrCmd(kGtestFilter, cdt.last_usr_cmd)) return;
    if (!IsCmdArgInRange(cdt.last_usr_cmd, cdt.tasks)) {
        DisplayListOfTasks(cdt.tasks, cdt);
    } else {
        std::string filter = ReadInputFromStdin(kGtestFilterArg + "=", cdt);
        int task_id = cdt.last_usr_cmd.arg - 1;
        Entity entity = CreateEntity(cdt);
        Execution& exec = cdt.execs[entity];
        exec.name = filter;
        exec.is_pinned = true;
        exec.task_id = task_id;
        Process& proc = cdt.processes[entity];
        proc.shell_command = GtestTaskCommandToShellCommand(cdt.tasks[task_id].command, filter);
        proc.stream_output = true;
        cdt.gtest_execs[entity] = GtestExecution{};
        cdt.execs_to_schedule.push_back(entity);
    }
}

void DisplayGtestExecutionResult(Cdt& cdt) {
    for (Entity entity: cdt.running_execs) {
        Execution& exec = cdt.execs[entity];
        Process& proc = cdt.processes[entity];
        ExecutionOutput& exec_output = cdt.exec_outputs[entity];
        GtestExecution* gtest_exec = Find(entity, cdt.gtest_execs);
        if (gtest_exec && !gtest_exec->rerun_of_single_test && exec.state != ExecutionState::kRunning && gtest_exec->state != GtestExecutionState::kFinished) {
            cdt.os->Out() << "\33[2K\r"; // Current line has test execution progress displayed. We will start displaying our output on top of it.
            if (gtest_exec->state == GtestExecutionState::kRunning) {
                exec.state = ExecutionState::kFailed;
                cdt.os->Out() << kTcRed << "'" << GtestTaskCommandToShellCommand(cdt.tasks[exec.task_id].command) << "' is not a google test executable" << kTcReset << std::endl;
            } else if (gtest_exec->state == GtestExecutionState::kParsing) {
                exec.state = ExecutionState::kFailed;
                cdt.os->Out() << kTcRed << "Tests have finished prematurely" << kTcReset << std::endl;
                // Tests might have crashed in the middle of some test. If so - consider test failed.
                // This is not always true however: tests might have crashed in between two test cases.
                if (gtest_exec->current_test) {
                    gtest_exec->failed_test_ids.push_back(*gtest_exec->current_test);
                    gtest_exec->current_test.reset();
                }
            } else if (gtest_exec->failed_test_ids.empty() && proc.stream_output) {
                cdt.os->Out() << kTcGreen << "Successfully executed " << gtest_exec->tests.size() << " tests "  << gtest_exec->total_duration << kTcReset << std::endl;
            }
            if (!gtest_exec->failed_test_ids.empty()) {
                PrintFailedGtestList(*gtest_exec, cdt);
                if (gtest_exec->failed_test_ids.size() == 1) {
                    GtestTest& test = gtest_exec->tests[gtest_exec->failed_test_ids[0]];
                    std::vector<std::string>& gtest_buffer = cdt.text_buffers[entity][TextBufferType::kGtest];
                    std::vector<std::string>& out_buffer = cdt.text_buffers[entity][TextBufferType::kOutput];
                    PrintGtestOutput(exec_output, gtest_buffer, out_buffer, test, kTcRed);
                }
            }
            gtest_exec->state = GtestExecutionState::kFinished;
        }
    }
}

void RestartRepeatingGtestOnSuccess(Cdt& cdt) {
    for (Entity entity: cdt.running_execs) {
        Execution& exec = cdt.execs[entity];
        GtestExecution* gtest_exec = Find(entity, cdt.gtest_execs);
        if (gtest_exec && exec.state == ExecutionState::kComplete && exec.repeat_until_fail) {
            cdt.gtest_execs[entity] = GtestExecution{gtest_exec->rerun_of_single_test};
            cdt.text_buffers[entity][TextBufferType::kGtest].clear();
        }
    }
}

void FinishGtestExecution(Cdt& cdt) {
    bool unpin_existing_entities = false;
    for (Entity entity: cdt.running_execs) {
        GtestExecution* gtest_exec = Find(entity, cdt.gtest_execs);
        if (!gtest_exec || cdt.execs[entity].state == ExecutionState::kRunning) {
            continue;
        }
        if (gtest_exec->rerun_of_single_test) {
            cdt.gtest_execs.erase(entity);
        } else {
            unpin_existing_entities = true;
        }
    }
    if (unpin_existing_entities) {
        for (Entity entity: cdt.exec_history) {
            if (Find(entity, cdt.gtest_execs)) {
                cdt.execs[entity].is_pinned = false;
            }
        }
    }
}
