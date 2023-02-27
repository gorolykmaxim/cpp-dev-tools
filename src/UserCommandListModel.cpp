#include "UserCommandListModel.hpp"

QString UserCommand::GetFormattedShortcut() const {
  QString result = shortcut.toUpper();
#if __APPLE__
  result.replace("CTRL", "\u2318");
#endif
  return result;
}

UserCommandListModel::UserCommandListModel(QObject* parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "group"}, {1, "name"}, {2, "shortcut"}, {3, "callback"}});
}

QVariantList UserCommandListModel::GetRow(int i) const {
  const UserCommand& cmd = list[i];
  return {cmd.group, cmd.name, cmd.shortcut, cmd.callback};
}

int UserCommandListModel::GetRowCount() const { return list.size(); }
