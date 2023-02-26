#include "UserCommandController.hpp"

#include "QVariantListModel.hpp"

UserCommandController::UserCommandController(QObject *parent)
    : QObject(parent), user_commands(new SimpleQVariantListModel(this)) {}

void UserCommandController::RegisterCommand(const QString &group,
                                            const QString &name,
                                            const QString &shortcut,
                                            QVariant callback) {
  user_commands->list.append({group, name, shortcut, callback});
}

void UserCommandController::Commit() { user_commands->Load(); }
