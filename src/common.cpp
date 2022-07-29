#include "common.h"
#include "cdt.h"
#include <string>
#include <vector>

std::string DefineUserCommand(const std::string& name,
                              const UserCommandDefinition& def, Cdt& cdt,
                              bool repeatable) {
  cdt.kUsrCmdNames.push_back(name);
  cdt.kUsrCmdDefs.push_back(def);
  if (repeatable) {
    cdt.kRepeatableUsrCmdNames.insert(name);
  }
  return name;
}

void WarnUserConfigPropNotSpecified(const std::string& property, Cdt& cdt) {
    cdt.os->Out() << kTcRed << '\'' << property << "' is not specified in "
                  << cdt.user_config_path.string() << kTcReset << std::endl;
}

std::string FormatTemplate(
    std::string str, const std::unordered_map<std::string, std::string>& vars) {
  int var_start = -1;
  for (int i = 0; i < str.size(); i++) {
    char c = str[i];
    if (c == '{') {
      var_start = i;
    } else if (c == '}' && var_start >= 0) {
      std::string var_name = str.substr(var_start + 1, i - var_start - 1);
      auto it = vars.find(var_name);
      if (it != vars.end()) {
        str.replace(var_start, i - var_start + 1, it->second);
        i = var_start - 1 + it->second.size();
      }
      var_start = -1;
    }
  }
  return str;
}

bool AcceptUsrCmd(const std::string& def, UserCommand& cmd) {
    if (!cmd.executed && def == cmd.cmd) {
        cmd.executed = true;
        return true;
    } else {
        return false;
    }
}

std::string ReadInputFromStdin(const std::string& prefix, Cdt& cdt) {
    cdt.os->Out() << kTcGreen << prefix << kTcReset;
    return cdt.os->ReadLineFromStdin();
}

void DisplayListOfTasks(const std::vector<Task>& tasks, Cdt& cdt) {
    cdt.os->Out() << kTcGreen << "Tasks:" << kTcReset << std::endl;
    for (int i = 0; i < tasks.size(); i++) {
        cdt.os->Out() << i + 1 << " \"" << tasks[i].name << "\"" << std::endl;
    }
}
