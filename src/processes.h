#ifndef PROCESSES_H
#define PROCESSES_H

#include "cdt.h"

void InitProcess(Cdt& cdt);
void StartProcesses(Cdt& cdt);
void HandleProcessEvent(Cdt& cdt);
void FinishProcessExecution(Cdt& cdt);

#endif
