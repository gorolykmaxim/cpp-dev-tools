#include "common.h"
#include <string>
#include <vector>

std::string DefineUserCommand(const std::string& name, const UserCommandDefinition& def, Cdt& cdt) {
    cdt.kUsrCmdNames.push_back(name);
    cdt.kUsrCmdDefs.push_back(def);
    return name;
}

Entity CreateEntity(Cdt& cdt) {
    return cdt.entity_seed++;
}

void DestroyEntity(Entity e, Cdt& cdt) {
    cdt.processes.erase(e);
    cdt.execs.erase(e);
    cdt.exec_outputs.erase(e);
    cdt.gtest_execs.erase(e);
    cdt.text_buffers.erase(e);
}

bool Find(Entity e, const std::unordered_set<Entity>& components) {
    return components.count(e) > 0;
}

void MoveTextBuffer(Entity e, TextBufferType from, TextBufferType to, std::unordered_map<Entity, std::unordered_map<TextBufferType, std::vector<std::string>>>& text_buffers) {
    std::vector<std::string>& f = text_buffers[e][from];
    std::vector<std::string>& t = text_buffers[e][to];
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
