#ifndef OUTPUT_H
#define OUTPUT_H

#include "cdt.h"

void InitOutput(Cdt& cdt);
void StreamExecutionOutput(Cdt& cdt);
void FindAndHighlightFileLinks(Cdt& cdt);
void PrintExecutionOutput(Cdt& cdt);
void StreamExecutionOutputOnFailure(Cdt& cdt);
void OpenFileLink(Cdt& cdt);
void SearchThroughLastExecutionOutput(Cdt& cdt);
void SearchThroughTextBuffer(Cdt& cdt);

#endif
