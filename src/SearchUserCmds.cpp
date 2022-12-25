#include "SearchUserCmds.hpp"
#include "Process.hpp"
#include "UI.hpp"

SearchUserCmds::SearchUserCmds() {
  EXEC_NEXT(DisplaySearchDialog);
  ON_CANCEL(Cancel);
}

void SearchUserCmds::DisplaySearchDialog(AppData& app) {
  QHash<int, QByteArray> role_names = {
    {0, "title"},
    {1, "subTitle"},
    {2, "rightText"},
    {3, "eventType"},
  };
  DisplayView(
      app,
      kDialogSlot,
      "SearchUserCmdsDialog.qml",
      {
        UIDataField{"dVisible", true},
        UIDataField{"dFixedHeight", true},
        UIDataField{"dPadding", false},
        UIDataField{"dFilter", ""},
      },
      {
        UIListField{"dCommands", role_names, MakeFilteredListOfCmds(app)},
      });
  WakeUpProcessOnUIEvent(app, kDialogSlot, "daFilterChanged", *this,
                         EXEC(this, FilterCommands));
  WakeUpProcessOnUIEvent(app, kDialogSlot, "daCancel", *this, EXEC(this, Noop));
  EXEC_NEXT(KeepAlive);
}

void SearchUserCmds::Cancel(AppData& app) {
  SetUIDataField(app, kDialogSlot, "dVisible", false);
}

void SearchUserCmds::FilterCommands(AppData& app) {
  filter = GetEventArg(app, 0).toString();
  QList<QVariantList> cmds = MakeFilteredListOfCmds(app);
  GetUIListField(app, kDialogSlot, "dCommands").SetItems(cmds);
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> SearchUserCmds::MakeFilteredListOfCmds(
    AppData& app) {
  QList<QVariantList> cmds;
  for (const QString& event_type: app.user_command_events_ordered) {
    UserCmd& cmd = app.user_cmds[event_type];
    QString name = cmd.name;
    QString group = cmd.group;
    bool name_matches = HighlightSubstring(name, filter);
    bool group_matches = HighlightSubstring(group, filter);
    if (filter.isEmpty() || name_matches || group_matches) {
      cmds.append({name, group, cmd.GetFormattedShortcut(), event_type});
    }
  }
  return cmds;
}
