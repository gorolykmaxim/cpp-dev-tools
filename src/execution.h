#ifndef EXECUTION_H
#define EXECUTION_H

#include "cdt.h"

void InitExecution(Cdt& cdt);
void ScheduleTask(Cdt& cdt);
void SchedulePreTasks(Cdt& cdt);
void ExecuteRestartTask(Cdt& cdt);
void StartNextExecution(Cdt& cdt);
void HandleProcessEvent(Cdt& cdt);
void DisplayExecutionResult(Cdt& cdt);
void RestartRepeatingExecutionOnSuccess(Cdt& cdt);
void FinishTaskExecution(Cdt& cdt);
void RemoveOldExecutionsFromHistory(Cdt& cdt);
void ValidateIfDebuggerAvailable(Cdt& cdt);
void AttachDebuggerToScheduledExecutions(Cdt& cdt);
void ChangeSelectedExecution(Cdt& cdt);

#endif
