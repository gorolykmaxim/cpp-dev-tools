#include "cdt.h"
#include "execution.h"
#include "output.h"
#include "common.h"
#include "config.h"
#include "gtest.h"
#include "user-command.h"

bool InitCdt(int argc, const char **argv, Cdt &cdt) {
    cdt.cdt_executable = argv[0];
    InitExecution(cdt);
    InitOutput(cdt);
    InitGtest(cdt);
    InitHelp(cdt);
    InitExampleUserConfig(cdt);
    ReadArgv(argc, argv, cdt);
    ReadUserConfig(cdt);
    if (PrintErrors(cdt)) return false;
    ReadTasksConfig(cdt);
    if (PrintErrors(cdt)) return false;
    ReadUserCommandFromEnv(cdt);
    PromptUserToAskForHelp(cdt);
    DisplayListOfTasks(cdt.tasks, cdt);
    return true;
}

bool WillWaitForInput(Cdt &cdt) {
    return cdt.execs_to_run_in_order.empty() && cdt.processes.empty();
}

void ExecCdtSystems(Cdt &cdt) {
    ReadUserCommandFromStdin(cdt);
    ValidateIfDebuggerAvailable(cdt);
    ScheduleTask(cdt);
    OpenFileLink(cdt);
    SearchThroughLastExecutionOutput(cdt);
    DisplayGtestOutput(cdt);
    SearchThroughGtestOutput(cdt);
    RerunGtest(cdt);
    ScheduleGtestTaskWithFilter(cdt);
    ChangeSelectedExecution(cdt);
    DisplayHelp(cdt);
    SearchThroughTextBuffer(cdt);
    ExecuteRestartTask(cdt);
    ScheduleGtestExecutions(cdt);
    ScheduleTaskExecutions(cdt);
    StartNextExecutionWithDebugger(cdt);
    StartNextExecution(cdt);
    ProcessExecutionEvent(cdt);
    ParseGtestOutput(cdt);
    DisplayGtestExecutionResult(cdt);
    StreamExecutionOutput(cdt);
    StreamExecutionOutputOnFailure(cdt);
    FindAndHighlightFileLinks(cdt);
    PrintExecutionOutput(cdt);
    DisplayExecutionResult(cdt);
    RestartRepeatingGtestOnSuccess(cdt);
    RestartRepeatingExecutionOnSuccess(cdt);
    FinishGtestExecution(cdt);
    FinishTaskExecution(cdt);
    RemoveOldExecutionsFromHistory(cdt);
}
