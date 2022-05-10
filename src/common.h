#ifndef COMMON_H
#define COMMON_H

#include "cdt.h"
#include <algorithm>
#include <deque>
#include <string>
#include <vector>

template <typename T>
void RemoveAll(const std::vector<T>& to_remove, std::deque<T>& from) {
    for (const T& t: to_remove) {
        auto it = std::find(from.begin(), from.end(), t);
        if (it != from.end()) {
            from.erase(it);
        }
    }
}

template <typename T>
T* Find(Entity e, std::unordered_map<Entity, T>& components) {
    auto res = components.find(e);
    return res != components.end() ? &res->second : nullptr;
}

template <typename T>
void DestroyComponents(const std::vector<Entity>& es, std::unordered_map<Entity, T>& components) {
    for (Entity e: es) {
        components.erase(e);
    }
}

template<typename T>
bool IsCmdArgInRange(const UserCommand& cmd, const T& range) {
    return cmd.arg > 0 && cmd.arg <= range.size();
}

std::string DefineUserCommand(const std::string& name, const UserCommandDefinition& def, Cdt& cdt);
Entity CreateEntity(Cdt& cdt);
void DestroyEntity(Entity e, Cdt& cdt);
bool Find(Entity e, const std::unordered_set<Entity>& components);
void MoveTextBuffer(Entity e, TextBufferType from, TextBufferType to, std::unordered_map<Entity, TextBuffer>& text_buffers);
void WarnUserConfigPropNotSpecified(const std::string& property, Cdt& cdt);
std::string FormatTemplate(const TemplateString& templ, const std::string& str);
bool AcceptUsrCmd(const std::string& def, UserCommand& cmd);
std::string ReadInputFromStdin(const std::string& prefix, Cdt& cdt);
void DisplayListOfTasks(const std::vector<Task>& tasks, Cdt& cdt);

#endif
