#ifndef COMMON_H
#define COMMON_H

#include "cdt.h"
#include <algorithm>
#include <deque>
#include <string>
#include <vector>

template<typename T>
bool IsCmdArgInRange(const UserCommand& cmd, const T& range) {
    return cmd.arg > 0 && cmd.arg <= range.size();
}

std::string DefineUserCommand(const std::string& name, const UserCommandDefinition& def, Cdt& cdt);
void MoveTextBuffer(TextBufferType from, TextBufferType to, TextBuffer& buffer);
void WarnUserConfigPropNotSpecified(const std::string& property, Cdt& cdt);
std::string FormatTemplate(const TemplateString& templ, const std::string& str);
bool AcceptUsrCmd(const std::string& def, UserCommand& cmd);
std::string ReadInputFromStdin(const std::string& prefix, Cdt& cdt);
void DisplayListOfTasks(const std::vector<Task>& tasks, Cdt& cdt);

#endif
