#include "DisplayExec.hpp"

#include "Process.hpp"
#include "Task.hpp"
#include "UI.hpp"

#define LOG() qDebug() << "[DisplayExec]"

static const QString kSubTextColor = "#6d6d6d";

struct UIIcon {
  QString icon;
  QString color;
};

static UIIcon MakeExecStatusIcon(const Exec& exec) {
  UIIcon icon;
  if (!exec.exit_code) {
    if (!exec.proc) {
      icon.icon = "access_alarm";
      icon.color = kSubTextColor;
    } else {
      icon.icon = "autorenew";
      icon.color = "green";
    }
  } else {
    if (*exec.exit_code == 0) {
      icon.icon = "check";
      icon.color = "";
    } else {
      icon.icon = "error";
      icon.color = "red";
    }
  }
  return icon;
}

DisplayExec::DisplayExec() { EXEC_NEXT(Display); }

void DisplayExec::Display(AppData& app) {
  QHash<int, QByteArray> role_names = {{0, "title"},     {1, "subTitle"},
                                       {2, "id"},        {3, "icon"},
                                       {4, "iconColor"}, {5, "isSelected"}};
  DisplayView(
      app, kViewSlot, "ExecView.qml",
      {UIDataField{"windowTitle", "Task Executions"},
       UIDataField{"vExecFilter", ""}, UIDataField{"vExecIcon", ""},
       UIDataField{"vExecIconColor", ""}, UIDataField{"vExecName", ""},
       UIDataField{"vExecCmd", ""}, UIDataField{"vExecOutput", ""}},
      {UIListField{"vExecs", role_names, MakeFilteredListOfExecs(app)}});
  WakeUpProcessOnUIEvent(app, kViewSlot, "execHistoryChanged", *this,
                         EXEC(this, ReDrawExecHistory));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecFilterChanged", *this,
                         EXEC(this, FilterExecs));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecSelected", *this,
                         EXEC(this, SelectExec));
}

void DisplayExec::ReDrawExecHistory(AppData& app) {
  QList<QVariantList> execs = MakeFilteredListOfExecs(app);
  GetUIListField(app, kViewSlot, "vExecs").SetItems(execs);
  EXEC_NEXT(KeepAlive);
}

void DisplayExec::FilterExecs(AppData& app) {
  exec_filter = GetEventArg(app, 0).toString();
  ReDrawExecHistory(app);
}

void DisplayExec::SelectExec(AppData& app) {
  selected_exec_id = GetEventArg(app, 0).toUuid();
  LOG() << "execution selected:" << selected_exec_id;
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> DisplayExec::MakeFilteredListOfExecs(AppData& app) {
  QList<QVariantList> rows;
  for (const Exec& exec : app.execs) {
    UIIcon icon = MakeExecStatusIcon(exec);
    QString cmd = ShortenTaskCmd(exec.cmd, *app.current_project);
    AppendToUIListIfMatches(rows, exec_filter,
                            {cmd, exec.start_time.toString(), exec.id,
                             icon.icon, icon.color, false},
                            {0, 1});
  }
  return rows;
}
