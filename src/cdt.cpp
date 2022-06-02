#include "cdt.h"
#include "execution.h"
#include "processes.h"
#include "output.h"
#include "common.h"
#include "config.h"
#include "gtest.h"
#include "user-command.h"

bool InitCdt(int argc, const char **argv, Cdt &cdt) {
    cdt.cdt_executable = argv[0];
    cdt.os->Init();
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
    DisplayUsedConfigProfile(cdt);
    PromptUserToAskForHelp(cdt);
    DisplayListOfTasks(cdt.tasks, cdt);
    return true;
}

bool WillWaitForInput(Cdt &cdt) {
    return cdt.execs_to_run.empty() && cdt.registry.view<Process>().empty();
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

  SchedulePreTasks(cdt);
  ScheduleGtestExecutions(cdt);
  AttachDebuggerToScheduledExecutions(cdt);
  ExecuteRestartTask(cdt);
  StartNextExecution(cdt);
  StartProcesses(cdt);

  HandleProcessEvent(cdt);
  ParseGtestOutput(cdt);
  DisplayGtestExecutionResult(cdt);
  StreamExecutionOutput(cdt);
  FindAndHighlightFileLinks(cdt);
  PrintExecutionOutput(cdt);

  SearchThroughOutput(cdt);

  DisplayExecutionResult(cdt);
  RestartRepeatingGtestOnSuccess(cdt);
  RestartRepeatingExecutionOnSuccess(cdt);
  FinishGtestExecution(cdt);
  FinishTaskExecution(cdt);
  FinishProcessExecution(cdt);

  RemoveOldExecutionsFromHistory(cdt);
}
