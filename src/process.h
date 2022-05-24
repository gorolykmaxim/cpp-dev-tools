#ifndef PROCESS_H
#define PROCESS_H

#include "cdt.h"

void StartProcesses(Cdt& cdt);
void HandleProcessEvent(Cdt& cdt);
void FinishProcessExecution(Cdt& cdt);

#endif
