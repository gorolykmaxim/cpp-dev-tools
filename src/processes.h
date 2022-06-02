#ifndef PROCESSES_H
#define PROCESSES_H

#include "cdt.h"

void StartProcesses(Cdt& cdt);
void HandleProcessEvent(Cdt& cdt);
void FinishProcessExecution(Cdt& cdt);

#endif
