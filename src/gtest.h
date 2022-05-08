#ifndef GTEST_H
#define GTEST_H

#include "cdt.h"

void InitGtest(Cdt& cdt);
void ScheduleGtestExecutions(Cdt& cdt);
void ParseGtestOutput(Cdt& cdt);
void DisplayGtestOutput(Cdt& cdt);
void SearchThroughGtestOutput(Cdt& cdt);
void RerunGtest(Cdt& cdt);
void ScheduleGtestTaskWithFilter(Cdt& cdt);
void DisplayGtestExecutionResult(Cdt& cdt);
void RestartRepeatingGtestOnSuccess(Cdt& cdt);
void FinishGtestExecution(Cdt& cdt);

#endif
