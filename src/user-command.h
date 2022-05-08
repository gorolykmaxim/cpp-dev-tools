#ifndef USER_COMMAND_H
#define USER_COMMAND_H

#include "cdt.h"

void InitHelp(Cdt& cdt);
void ReadUserCommandFromStdin(Cdt& cdt);
void ReadUserCommandFromEnv(Cdt& cdt);
void PromptUserToAskForHelp(Cdt& cdt);
void DisplayHelp(Cdt& cdt);

#endif
