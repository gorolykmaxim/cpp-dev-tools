#include "ExecTasks.hpp"
#include "Process.hpp"
#include "UI.hpp"
#include "ExecTask.hpp"

ExecTasks::ExecTasks() {
  EXEC_NEXT(DisplayExecTasksView);
}

void ExecTasks::DisplayExecTasksView(AppData& app) {
  QHash<int, QByteArray> role_names = {
    {0, "title"},
    {1, "subTitle"},
    {2, "taskName"},
  };
  DisplayView(
      app,
      kViewSlot,
      "ExecTasksView.qml",
      {
        UIDataField{"windowTitle", "Execute Tasks"},
        UIDataField{"vTaskFilter", ""},
      },
      {
        UIListField{"vTasks", role_names, MakeFilteredListOfTasks(app)},
      });
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaTaskFilterChanged", *this,
                         EXEC(this, FilterTasks));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecuteTask", *this,
                         EXEC(this, ExecSelectedTask));
}

void ExecTasks::FilterTasks(AppData& app) {
  task_filter = GetEventArg(app, 0).toString();
  QList<QVariantList> tasks = MakeFilteredListOfTasks(app);
  GetUIListField(app, kViewSlot, "vTasks").SetItems(tasks);
  EXEC_NEXT(KeepAlive);
}

void ExecTasks::ExecSelectedTask(AppData& app) {
  QString name = GetEventArg(app, 0).toString();
  ScheduleProcess<ExecTask>(app, nullptr, name);
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
