#include <sstream>
#include <string>

#include "cdt.h"
#include "user-command.h"
#include "common.h"

static std::string kHelp;

void InitHelp(Cdt& cdt) {
    kHelp = DefineUserCommand("h", {"", "Display list of user commands"}, cdt);
}

static UserCommand ParseUserCommand(const std::string& str) {
    std::stringstream chars;
    std::stringstream digits;
    for (int i = 0; i < str.size(); i++) {
        char c = str[i];
        if (std::isspace(c)) {
            continue;
        }
        if (std::isdigit(c)) {
            digits << c;
        } else {
            chars << c;
        }
    }
    UserCommand cmd;
    cmd.cmd = chars.str();
    cmd.arg = std::atoi(digits.str().c_str());
    return cmd;
}

void ReadUserCommandFromStdin(Cdt& cdt) {
    if (!WillWaitForInput(cdt)) return;
    std::string input = ReadInputFromStdin("(cdt) ", cdt);
    if (input.empty()) {
        cdt.last_usr_cmd.executed = false;
    } else {
        cdt.last_usr_cmd = ParseUserCommand(input);
    }
}

void ReadUserCommandFromEnv(Cdt& cdt) {
    std::string str = cdt.os->GetEnv(kEnvVarLastCommand);
    if (!str.empty()) {
        cdt.last_usr_cmd = ParseUserCommand(str);
    }
}

void PromptUserToAskForHelp(Cdt& cdt) {
    cdt.os->Out() << "Type " << kTcGreen << kHelp << kTcReset << " to see list of all the user commands." << std::endl;
}

void DisplayHelp(Cdt& cdt) {
    if (cdt.last_usr_cmd.executed) return;
    cdt.last_usr_cmd.executed = true;
    cdt.os->Out() << kTcGreen << "User commands:" << kTcReset << std::endl;
    for (int i = 0; i < cdt.kUsrCmdNames.size(); i++) {
        std::string& name = cdt.kUsrCmdNames[i];
        UserCommandDefinition& def = cdt.kUsrCmdDefs[i];
        cdt.os->Out() << name;
        if (!def.arg.empty()) {
            cdt.os->Out() << '<' << def.arg << '>';
        }
        cdt.os->Out() << ((name.size() + def.arg.size()) < 6 ? "\t\t" : "\t") << def.desc << std::endl;
    }
}
