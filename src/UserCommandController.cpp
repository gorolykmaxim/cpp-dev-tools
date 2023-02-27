#include "UserCommandController.hpp"

#include "UserCommandListModel.hpp"

UserCommandController::UserCommandController(QObject *parent)
    : QObject(parent), user_commands(new UserCommandListModel(this)) {}

void UserCommandController::RegisterCommand(const QString &group,
                                            const QString &name,
                                            const QString &shortcut,
                                            QVariant callback) {
  UserCommand cmd;
  cmd.group = group;
  cmd.name = name;
  cmd.shortcut = shortcut;
  cmd.callback = callback;
  user_commands->list.append(cmd);
}

void UserCommandController::Commit() { user_commands->Load(); }
