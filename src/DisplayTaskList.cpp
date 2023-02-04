#include "DisplayTaskList.hpp"

#include "DisplayExec.hpp"
#include "ExecOSCmd.hpp"
#include "Process.hpp"
#include "Threads.hpp"
#include "UI.hpp"

#define LOG() qDebug() << "[DisplayTaskList]"

DisplayTaskList::DisplayTaskList() { EXEC_NEXT(FindTasks); }

static bool CompareExecs(const QString &a, const QString &b) {
  if (a.size() != b.size()) {
    return a.size() < b.size();
  } else {
    return a < b;
  }
}

void DisplayTaskList::FindTasks(AppData &app) {
  QPtr<DisplayTaskList> self = ProcessSharedPtr(app, this);
  QString path = app.current_project->path;
  ScheduleIOTask<QList<QString>>(
      app,
      [path]() {
        LOG() << "Searching for executables";
        QList<QString> results;
        QDirIterator it(path, QDir::Files | QDir::Executable,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
          results.append(it.next());
        }
        std::sort(results.begin(), results.end(), CompareExecs);
        return results;
      },
      [&app, self](QList<QString> execs) {
        LOG() << execs.size() << "executables found";
        self->execs = execs;
        WakeUpAndExecuteProcess(app, *self);
      });
  QHash<int, QByteArray> role_names = {{0, "idx"}, {1, "title"}};
  DisplayView(
      app, kViewSlot, "TaskListView.qml",
      {UIDataField{"windowTitle", "Tasks"}, UIDataField{"vFilter", ""},
       UIDataField{"vLoading", true}},
      {UIListField{"vTasks", role_names, MakeFilteredListOfTasks(app)}});
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaFilterChanged", *this,
                         EXEC(this, FilterTasks));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecuteTask", *this,
                         EXEC(this, ExecTask));
  EXEC_NEXT(Display);
}

void DisplayTaskList::Display(AppData &app) {
  QList<QVariantList> tasks = MakeFilteredListOfTasks(app);
  SetUIDataField(app, kViewSlot, "vLoading", false);
  GetUIListField(app, kViewSlot, "vTasks").SetItems(tasks);
  EXEC_NEXT(KeepAlive);
}

void DisplayTaskList::FilterTasks(AppData &app) {
  filter = GetEventArg(app, 0).toString();
  Display(app);
}

void DisplayTaskList::ExecTask(AppData &app) {
  int i = GetEventArg(app, 0).toInt();
  QString exec = execs[i];
  ScheduleProcess<ExecOSCmd>(app, nullptr, exec);
  ScheduleProcess<DisplayExec>(app, kViewSlot);
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> DisplayTaskList::MakeFilteredListOfTasks(AppData &app) {
  QList<QVariantList> rows;
  for (int i = 0; i < execs.size(); i++) {
    QString exec = execs[i];
    if (exec.startsWith(app.current_project->path)) {
      exec.replace(app.current_project->path, ".");
    }
    AppendToUIListIfMatches(rows, filter, {i, exec}, {1});
  }
  return rows;
}
