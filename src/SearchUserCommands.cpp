#include "SearchUserCommands.hpp"
#include "Process.hpp"
#include "UI.hpp"
#include "Common.hpp"

SearchUserCommands::SearchUserCommands() {
  EXEC_NEXT(DisplaySearchDialog);
  ON_CANCEL(Cancel);
}

void SearchUserCommands::DisplaySearchDialog(AppData& app) {
  QHash<int, QByteArray> role_names = {
    {0, "name"},
    {1, "group"},
    {2, "shortcut"},
    {3, "eventType"},
  };
  DisplayView(
      app,
      kDialogSlot,
      "SearchUserCommandsDialog.qml",
      {
        UIDataField{"dVisible", true},
        UIDataField{"dFixedHeight", true},
        UIDataField{"dPadding", false},
        UIDataField{"dFilter", ""},
      },
      {
        UIListField{"dCommands", role_names, MakeFilteredListOfCommands(app)},
      });
  WakeUpProcessOnUIEvent(app, kDialogSlot, "daFilterChanged", *this,
                         EXEC(this, FilterCommands));
  WakeUpProcessOnUIEvent(app, kDialogSlot, "daCancel", *this, EXEC(this, Noop));
  EXEC_NEXT(KeepAlive);
}

void SearchUserCommands::Cancel(AppData& app) {
  SetUIDataField(app, kDialogSlot, "dVisible", false);
}

void SearchUserCommands::FilterCommands(AppData& app) {
  filter = GetEventArg(app, 0).toString();
  QList<QVariantList> cmds = MakeFilteredListOfCommands(app);
  GetUIListField(app, kDialogSlot, "dCommands").SetItems(cmds);
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> SearchUserCommands::MakeFilteredListOfCommands(
    AppData& app) {
  QList<QVariantList> cmds;
  for (const QString& event_type: app.user_commands.keys()) {
    UserCommand& cmd = app.user_commands[event_type];
    QString name = cmd.name;
    QString group = cmd.group;
    bool name_matches = HighlighSubstring(name, filter);
    bool group_matches = HighlighSubstring(group, filter);
    if (filter.isEmpty() || name_matches || group_matches) {
      cmds.append({name, group, cmd.GetFormattedShortcut(), event_type});
    }
  }
  return cmds;
}
