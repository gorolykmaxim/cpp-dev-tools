#ifndef CONFIG_H
#define CONFIG_H

#include "cdt.h"

void InitExampleUserConfig(Cdt& cdt);
bool ReadArgv(int argc, const char** argv, Cdt& cdt);
void ReadUserConfig(Cdt& cdt);
void ReadTasksConfig(Cdt& cdt);
bool PrintErrors(const Cdt& cdt);

#endif
