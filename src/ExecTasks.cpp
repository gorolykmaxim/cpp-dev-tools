#include "ExecTasks.hpp"
#include "Process.hpp"
#include "UI.hpp"

ExecTasks::ExecTasks() {
  EXEC_NEXT(DisplayExecTasksView);
}

void ExecTasks::DisplayExecTasksView(AppData& app) {
  QHash<int, QByteArray> role_names = {
    {0, "title"},
    {1, "subTitle"},
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
}

void ExecTasks::FilterTasks(AppData& app) {
  taskFilter = GetEventArg(app, 0).toString();
  QList<QVariantList> tasks = MakeFilteredListOfTasks(app);
  GetUIListField(app, kViewSlot, "vTasks").SetItems(tasks);
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> ExecTasks::MakeFilteredListOfTasks(AppData& app) {
  QList<QVariantList> tasks;
  for (const Task& task: app.tasks) {
    QString name = task.name;
    QString command = task.command;
    bool name_matches = HighlightSubstring(name, taskFilter);
    bool command_matches = HighlightSubstring(command, taskFilter);
    if (taskFilter.isEmpty() || name_matches || command_matches) {
      tasks.append({name, command});
    }
  }
  return tasks;
}
