#include "common.h"
#include "cdt.h"
#include <string>
#include <vector>

std::string DefineUserCommand(const std::string& name, const UserCommandDefinition& def, Cdt& cdt) {
    cdt.kUsrCmdNames.push_back(name);
    cdt.kUsrCmdDefs.push_back(def);
    return name;
}

void MoveTextBuffer(TextBufferType from, TextBufferType to, TextBuffer& buffer) {
    std::vector<std::string>& f = buffer.buffers[from];
    std::vector<std::string>& t = buffer.buffers[to];
    t.reserve(t.size() + f.size());
    t.insert(t.end(), f.begin(), f.end());
    f.clear();
}

void WarnUserConfigPropNotSpecified(const std::string& property, Cdt& cdt) {
    cdt.os->Out() << kTcRed << '\'' << property << "' is not specified in " << cdt.user_config_path << kTcReset << std::endl;
}

std::string FormatTemplate(const TemplateString& templ, const std::string& str) {
    std::string res = templ.str;
    res.replace(templ.arg_pos, kTemplateArgPlaceholder.size(), str);
    return res;
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
