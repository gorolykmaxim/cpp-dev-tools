#include "common.h"
#include "cdt.h"
#include <string>
#include <vector>

std::string DefineUserCommand(const std::string& name, const UserCommandDefinition& def, Cdt& cdt) {
    cdt.kUsrCmdNames.push_back(name);
    cdt.kUsrCmdDefs.push_back(def);
    return name;
}

void WarnUserConfigPropNotSpecified(const std::string& property, Cdt& cdt) {
    cdt.os->Out() << kTcRed << '\'' << property << "' is not specified in " << cdt.user_config_path << kTcReset << std::endl;
}

std::string FormatTemplate(std::string str, const std::string& substr,
                           const std::string& replacement) {
    std::string::size_type p = str.find(substr);
    if (p != std::string::npos) {
        str.replace(p, substr.size(), replacement);
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
    std::string input;
    std::getline(cdt.os->In(), input);
    return input;
}

void DisplayListOfTasks(const std::vector<Task>& tasks, Cdt& cdt) {
    cdt.os->Out() << kTcGreen << "Tasks:" << kTcReset << std::endl;
    for (int i = 0; i < tasks.size(); i++) {
        cdt.os->Out() << i + 1 << " \"" << tasks[i].name << "\"" << std::endl;
    }
}
