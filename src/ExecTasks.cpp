#include "ExecTasks.hpp"
#include "Process.hpp"
#include "UI.hpp"
#include "Execution.hpp"
#include "ExecTask.hpp"

#define LOG() qDebug() << "[ExecTasks]"

static const QString kSubTextColor = "#6d6d6d";

struct UIIcon {
  QString icon;
  QString color;
};

static QString MakeLabelAndValue(const QString& label, const QString& value) {
  return label + ": <b>" + value + "<b/>";
}

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

static void DisplayExecOutput(AppData& app, QUuid exec_id) {
  Exec* exec = FindExecById(app, exec_id);
  if (!exec) {
    return;
  }
  UIIcon icon = MakeExecStatusIcon(*exec);
  SetUIDataField(app, kViewSlot, "vExecName",
                 MakeLabelAndValue("Task Name", exec->task_name));
  SetUIDataField(app, kViewSlot, "vExecCmd",
                 MakeLabelAndValue("Command", exec->cmd));
  SetUIDataField(app, kViewSlot, "vExecIcon", icon.icon);
  SetUIDataField(app, kViewSlot, "vExecIconColor", icon.color);
  SetUIDataField(app, kViewSlot, "vExecOutput", exec->output);
}

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
    {6, "titleColor"},
    {7, "isSelected"},
  };
  DisplayView(
      app,
      kViewSlot,
      "ExecTasksView.qml",
      {
        UIDataField{"windowTitle", "Execute Tasks"},
        UIDataField{"vTaskFilter", ""},
        UIDataField{"vExecFilter", ""},
        UIDataField{"vExecName", ""},
        UIDataField{"vExecOutput", ""},
        UIDataField{"vExecCmd", ""},
        UIDataField{"vExecIcon", ""},
        UIDataField{"vExecIconColor", ""},
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
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecSelected", *this,
                         EXEC(this, ExecSelected));
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
  EXEC_NEXT_DEFERRED(KeepAlive);
  QUuid id = GetEventArg(app, 0).toUuid();
  if (selected_exec_id != id) {
    return;
  }
  DisplayExecOutput(app, selected_exec_id);
}

void ExecTasks::ReDrawExecHistory(AppData& app) {
  if (select_new_execs) {
    if (Exec* exec = FindLatestRunningExec(app)) {
      selected_exec_id = exec->id;
    } else if (!app.execs.isEmpty()) {
      selected_exec_id = app.execs.last().id;
    }
    DisplayExecOutput(app, selected_exec_id);
  }
  QList<QVariantList> execs = MakeFilteredListOfExecs(app);
  GetUIListField(app, kViewSlot, "vExecs").SetItems(execs);
  EXEC_NEXT(KeepAlive);
}

void ExecTasks::ExecSelected(AppData& app) {
  selected_exec_id = GetEventArg(app, 0).toUuid();
  Exec* exec = FindLatestRunningExec(app);
  bool is_latest_running = exec && exec->id == selected_exec_id;
  bool is_latest = !app.execs.isEmpty() &&
                   app.execs.last().id == selected_exec_id;
  select_new_execs = is_latest_running || is_latest;
  if (select_new_execs) {
    LOG() << "latest execution will get selected:" << selected_exec_id;
  } else {
    LOG() << "execution selected:" << selected_exec_id;
  }
  DisplayExecOutput(app, selected_exec_id);
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
  QList<QVariantList> execs;
  for (Exec& exec: app.execs) {
    UIIcon icon = MakeExecStatusIcon(exec);
    QString title_color = exec.id == exec.primary_exec_id ? "" : kSubTextColor;
    bool is_selected = exec.id == selected_exec_id;
    AppendToUIListIfMatches(execs, exec_filter, {exec.task_name,
                            exec.start_time.toString(), exec.id, exec.cmd,
                            icon.icon, icon.color, title_color, is_selected},
                            {0, 1, 3});
  }
  return execs;
}
