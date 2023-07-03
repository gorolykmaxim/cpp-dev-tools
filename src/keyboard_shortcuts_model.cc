#include "keyboard_shortcuts_model.h"

#include "database.h"
#include "io_task.h"
#include "theme.h"

#define LOG() qDebug() << "[KeyboardShortcutsModel]"

KeyboardShortcutsModel::KeyboardShortcutsModel(QObject* parent)
    : TextListModel(parent), selected_command(-1) {
  SetRoleNames({{0, "title"}, {1, "rightText"}, {2, "rightTextColor"}});
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

QList<Database::Cmd> KeyboardShortcutsModel::MakeCommandsToUpdateDatabase()
    const {
  QList<Database::Cmd> cmds;
  for (int i : new_shortcut.keys()) {
    const UserCommand& cmd = list[i];
    cmds.append(Database::Cmd(
        "UPDATE user_command SET shortcut=? WHERE \"group\"=? AND name=?",
        {new_shortcut[i], cmd.group, cmd.name}));
  }
  return cmds;
}

void KeyboardShortcutsModel::ResetAllModifications() {
  selected_command = -1;
  new_shortcut.clear();
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
            "SELECT \"group\", name, shortcut FROM user_command "
            "ORDER BY global DESC, \"group\", name",
            &UserCommandSystem::ReadUserCommandFromSql);
      },
      [this](QList<UserCommand> cmds) {
        list = cmds;
        Load(-1);
      });
}

void KeyboardShortcutsModel::setSelectedShortcut(const QString& shortcut) {
  if (selected_command < 0) {
    return;
  }
  LOG() << "Selection action has new shortcut:" << shortcut;
  new_shortcut[selected_command] = shortcut;
  Load(-1);
}

QVariantList KeyboardShortcutsModel::GetRow(int i) const {
  static const Theme kTheme;
  const UserCommand& cmd = list[i];
  QString shortcut = cmd.shortcut;
  QString color;
  if (new_shortcut.contains(i)) {
    shortcut = new_shortcut[i];
    color = kTheme.kColorPrimary;
  }
  return {cmd.group + " > " + cmd.name, shortcut, color};
}

int KeyboardShortcutsModel::GetRowCount() const { return list.size(); }
