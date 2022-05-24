#ifndef PROCESS_H
#define PROCESS_H

#include "cdt.h"

void InitProcess(Cdt& cdt);
void StartProcesses(Cdt& cdt);
void HandleProcessEvent(Cdt& cdt);
void FinishProcessExecution(Cdt& cdt);

#endif
