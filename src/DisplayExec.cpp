#include "DisplayExec.hpp"

#include "Process.hpp"
#include "Task.hpp"
#include "UI.hpp"

#define LOG() qDebug() << "[DisplayExec]"

static const QString kSubTextColor = "#6d6d6d";

static void DisplayExecOutput(AppData& app, const QUuid& exec_id) {
  Exec* exec = FindExecById(app, exec_id);
  if (!exec) {
    return;
  }
  QString status = "Running...";
  if (exec->exit_code) {
    status = "Exit Code: " + QString::number(*exec->exit_code);
  }
  SetUIDataField(app, kViewSlot, "vExecCmd",
                 ShortenTaskCmd(exec->cmd, *app.current_project));
  SetUIDataField(app, kViewSlot, "vExecStatus", status);
  SetUIDataField(app, kViewSlot, "vExecOutput", exec->output);
}

DisplayExec::DisplayExec() { EXEC_NEXT(Display); }

void DisplayExec::Display(AppData& app) {
  QHash<int, QByteArray> role_names = {{0, "title"},     {1, "subTitle"},
                                       {2, "id"},        {3, "icon"},
                                       {4, "iconColor"}, {5, "isSelected"}};
  DisplayView(
      app, kViewSlot, "ExecView.qml",
      {UIDataField{"windowTitle", "Task Execution History"},
       UIDataField{"vExecFilter", ""}, UIDataField{"vExecCmd", ""},
       UIDataField{"vExecStatus", ""}, UIDataField{"vExecOutput", ""},
       UIDataField{"vExecsEmpty", app.execs.isEmpty()}},
      {UIListField{"vExecs", role_names, MakeFilteredListOfExecs(app)}});
  WakeUpProcessOnUIEvent(app, kViewSlot, "execHistoryChanged", *this,
                         EXEC(this, ReDrawExecHistory));
  WakeUpProcessOnUIEvent(app, kViewSlot, "execOutputChanged", *this,
                         EXEC(this, ReDrawExecOutput));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecFilterChanged", *this,
                         EXEC(this, FilterExecs));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecSelected", *this,
                         EXEC(this, SelectExec));
}

void DisplayExec::ReDrawExecHistory(AppData& app) {
  if (select_new_execs) {
    if (Exec* exec = FindLatestRunningExec(app)) {
      selected_exec_id = exec->id;
    } else if (!app.execs.isEmpty()) {
      selected_exec_id = app.execs.last().id;
    }
    DisplayExecOutput(app, selected_exec_id);
  }
  QList<QVariantList> execs = MakeFilteredListOfExecs(app);
  SetUIDataField(app, kViewSlot, "vExecsEmpty", app.execs.isEmpty());
  GetUIListField(app, kViewSlot, "vExecs").SetItems(execs);
  EXEC_NEXT(KeepAlive);
}

void DisplayExec::ReDrawExecOutput(AppData& app) {
  EXEC_NEXT(KeepAlive);
  QUuid id = GetEventArg(app, 0).toUuid();
  if (selected_exec_id != id) {
    return;
  }
  DisplayExecOutput(app, selected_exec_id);
}

void DisplayExec::FilterExecs(AppData& app) {
  exec_filter = GetEventArg(app, 0).toString();
  ReDrawExecHistory(app);
}

void DisplayExec::SelectExec(AppData& app) {
  selected_exec_id = GetEventArg(app, 0).toUuid();
  Exec* exec = FindLatestRunningExec(app);
  bool is_latest_running = exec && exec->id == selected_exec_id;
  bool is_latest =
      !app.execs.isEmpty() && app.execs.last().id == selected_exec_id;
  select_new_execs = is_latest_running || is_latest;
  if (select_new_execs) {
    LOG() << "latest execution will get selected:" << selected_exec_id;
  } else {
    LOG() << "execution selected:" << selected_exec_id;
  }
  DisplayExecOutput(app, selected_exec_id);
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> DisplayExec::MakeFilteredListOfExecs(AppData& app) {
  QList<QVariantList> rows;
  for (const Exec& exec : app.execs) {
    QString icon, iconColor;
    if (!exec.exit_code) {
      if (!exec.proc) {
        icon = "access_alarm";
        iconColor = kSubTextColor;
      } else {
        icon = "autorenew";
        iconColor = "green";
      }
    } else {
      if (*exec.exit_code == 0) {
        icon = "check";
        iconColor = "";
      } else {
        icon = "error";
        iconColor = "red";
      }
    }
    QString cmd = ShortenTaskCmd(exec.cmd, *app.current_project);
    bool is_selected = exec.id == selected_exec_id;
    AppendToUIListIfMatches(rows, exec_filter,
                            {cmd, exec.start_time.toString(), exec.id, icon,
                             iconColor, is_selected},
                            {0, 1});
  }
  return rows;
}
