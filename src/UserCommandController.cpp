#include "UserCommandController.hpp"

#include "QVariantListModel.hpp"

UserCommandListModel::UserCommandListModel(QObject *parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "group"}, {1, "name"}, {2, "shortcut"}, {3, "callback"}});
}

QVariantList UserCommandListModel::GetRow(int i) const { return list[i]; }

int UserCommandListModel::GetRowCount() const { return list.size(); }

UserCommandController::UserCommandController(QObject *parent)
    : QObject(parent), user_commands(new UserCommandListModel(this)) {}

void UserCommandController::RegisterCommand(const QString &group,
                                            const QString &name,
                                            const QString &shortcut,
                                            QVariant callback) {
  user_commands->list.append({group, name, shortcut, callback});
}

void UserCommandController::Commit() { user_commands->Load(); }
