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
  if (selected_command < 0) {
    return "";
  }
  if (new_shortcut.contains(selected_command)) {
    return new_shortcut[selected_command];
  }
  return list[selected_command].shortcut;
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
  if (list[selected_command].shortcut == shortcut) {
    new_shortcut.remove(selected_command);
  } else {
    new_shortcut[selected_command] = shortcut;
  }
  Load(-1);
}

void KeyboardShortcutsModel::resetCurrentShortcut() {
  int i = GetSelectedItemIndex();
  LOG() << "Reseting modification to shortcut" << i;
  if (new_shortcut.remove(i)) {
    Load(-1);
  }
}

void KeyboardShortcutsModel::resetAllShortcuts() {
  LOG() << "Reseting all shortcut modifications";
  bool reload_after = !new_shortcut.isEmpty();
  new_shortcut.clear();
  if (reload_after) {
    Load(-1);
  }
}

void KeyboardShortcutsModel::restoreDefaultOfCurrentShortcut() {
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  RestoreDefaultOfShortcut(i);
  Load(-1);
}

void KeyboardShortcutsModel::restoreDefaultOfAllShortcuts() {
  LOG() << "Restoring all shortcuts to their default values";
  for (int i = 0; i < list.size(); i++) {
    RestoreDefaultOfShortcut(i);
  }
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

void KeyboardShortcutsModel::RestoreDefaultOfShortcut(int i) {
  const UserCommand& cmd = list[i];
  Application& app = Application::Get();
  QString shortcut = app.user_command.GetDefaultShortcut(cmd.group, cmd.name);
  if ((new_shortcut.contains(i) && new_shortcut[i] == shortcut) ||
      (!new_shortcut.contains(i) && shortcut == cmd.shortcut)) {
    return;
  }
  LOG() << "Restoring shortcut" << i << "to its default value";
  new_shortcut[i] = shortcut;
  if (i == selected_command) {
    emit selectedCommandChanged();
  }
}
