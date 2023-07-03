#include "keyboard_shortcuts_model.h"

#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[KeyboardShortcutsModel]"

KeyboardShortcutsModel::KeyboardShortcutsModel(QObject* parent)
    : TextListModel(parent), selected_command(-1) {
  SetRoleNames({{0, "title"}, {1, "rightText"}});
  searchable_roles = {0, 1};
}

static QString FormatName(const UserCommand& cmd) {
  return cmd.group + " > " + cmd.name;
}

QString KeyboardShortcutsModel::GetSelectedCommand() const {
  if (selected_command < 0) {
    return "Select command to modify shortcut";
  } else {
    return FormatName(list[selected_command]);
  }
}

QString KeyboardShortcutsModel::GetSelectedShortcut() const {
  return selected_command < 0 ? "" : list[selected_command].shortcut;
}

void KeyboardShortcutsModel::selectCurrentCommand() {
  selected_command = GetSelectedItemIndex();
  LOG() << "Selected command for editing" << selected_command;
  emit selectedCommandChanged();
}

void KeyboardShortcutsModel::load() {
  LOG() << "Loading user commands";
  IoTask::Run<QList<UserCommand>>(
      this,
      [] {
        return Database::ExecQueryAndRead<UserCommand>(
            "SELECT \"group\", name, shortcut FROM user_command ORDER BY "
            "global DESC, \"group\", name",
            &UserCommandSystem::ReadUserCommandFromSql);
      },
      [this](QList<UserCommand> cmds) {
        list = cmds;
        Load(-1);
      });
}

QVariantList KeyboardShortcutsModel::GetRow(int i) const {
  const UserCommand& cmd = list[i];
  return {cmd.group + " > " + cmd.name, cmd.shortcut};
}

int KeyboardShortcutsModel::GetRowCount() const { return list.size(); }
