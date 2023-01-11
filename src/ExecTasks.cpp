#include "ExecTasks.hpp"
#include "Process.hpp"
#include "UI.hpp"
#include "ExecTask.hpp"

ExecTasks::ExecTasks() {
  EXEC_NEXT(DisplayExecTasksView);
}

void ExecTasks::DisplayExecTasksView(AppData& app) {
  QHash<int, QByteArray> task_role_names = {
    {0, "title"},
    {1, "subTitle"},
    {2, "taskName"},
  };
  QHash<int, QByteArray> exec_role_names = {
    {0, "title"},
    {1, "subTitle"},
    {2, "id"},
    {3, "cmd"},
    {4, "icon"},
    {5, "iconColor"},
  };
  DisplayView(
      app,
      kViewSlot,
      "ExecTasksView.qml",
      {
        UIDataField{"windowTitle", "Execute Tasks"},
        UIDataField{"vTaskFilter", ""},
        UIDataField{"vExecFilter", ""},
      },
      {
        UIListField{"vTasks", task_role_names, MakeFilteredListOfTasks(app)},
        UIListField{"vExecs", exec_role_names, MakeFilteredListOfExecs(app)},
      });
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaTaskFilterChanged", *this,
                         EXEC(this, FilterTasks));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecFilterChanged", *this,
                         EXEC(this, FilterExecs));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecuteTask", *this,
                         EXEC(this, ExecSelectedTask));
  WakeUpProcessOnUIEvent(app, kViewSlot, "execOutputChanged", *this,
                         EXEC(this, ReDrawExecOutput));
  WakeUpProcessOnUIEvent(app, kViewSlot, "execHistoryChanged", *this,
                         EXEC(this, ReDrawExecHistory));
}

void ExecTasks::FilterTasks(AppData& app) {
  task_filter = GetEventArg(app, 0).toString();
  QList<QVariantList> tasks = MakeFilteredListOfTasks(app);
  GetUIListField(app, kViewSlot, "vTasks").SetItems(tasks);
  EXEC_NEXT(KeepAlive);
}

void ExecTasks::FilterExecs(AppData& app) {
  exec_filter = GetEventArg(app, 0).toString();
  ReDrawExecHistory(app);
}

void ExecTasks::ExecSelectedTask(AppData& app) {
  QString name = GetEventArg(app, 0).toString();
  ScheduleProcess<ExecTask>(app, nullptr, name);
  EXEC_NEXT(KeepAlive);
}

void ExecTasks::ReDrawExecOutput(AppData& app) {
  qDebug() << "OUTPUT OF EXEC CHANGED" << GetEventArg(app, 0).toUuid();
  EXEC_NEXT(KeepAlive);
}

void ExecTasks::ReDrawExecHistory(AppData& app) {
  QList<QVariantList> execs = MakeFilteredListOfExecs(app);
  GetUIListField(app, kViewSlot, "vExecs").SetItems(execs);
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> ExecTasks::MakeFilteredListOfTasks(AppData& app) {
  QList<QVariantList> tasks;
  for (Task& task: app.tasks) {
    AppendToUIListIfMatches(tasks, task_filter, {task.name, task.cmd,
                            task.name}, {0, 1});
  }
  return tasks;
}

QList<QVariantList> ExecTasks::MakeFilteredListOfExecs(AppData& app) {
  static const QString sub_text_color = "#6d6d6d";
  QList<QVariantList> execs;
  for (Exec& exec: app.execs) {
    QString icon;
    QString icon_color;
    if (!exec.exit_code) {
      if (!exec.proc) {
        icon = "access_alarm";
        icon_color = sub_text_color;
      } else {
        icon = "autorenew";
        icon_color = "green";
      }
    } else {
      if (*exec.exit_code == 0) {
        icon = "check";
        icon_color = "";
      } else {
        icon = "error";
        icon_color = "red";
      }
    }
    AppendToUIListIfMatches(execs, exec_filter, {exec.task_name,
                            exec.start_time.toString(), exec.id, exec.cmd,
                            icon, icon_color}, {0, 1, 3});
  }
  return execs;
}
