#include "ChooseTask.hpp"

#define LOG() qDebug() << "[ChooseTask]"

#include "Process.hpp"
#include "Task.hpp"
#include "Threads.hpp"
#include "UI.hpp"

ChooseTask::ChooseTask() : window_title("Choose Task") { EXEC_NEXT(FindTasks); }

static bool CompareExecs(const QString &a, const QString &b) {
  if (a.size() != b.size()) {
    return a.size() < b.size();
  } else {
    return a < b;
  }
}

void ChooseTask::FindTasks(AppData &app) {
  QPtr<ChooseTask> self = ProcessSharedPtr(app, this);
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
      app, kViewSlot, "ChooseTaskView.qml",
      {UIDataField{"windowTitle", window_title}, UIDataField{"vFilter", ""},
       UIDataField{"vLoading", true}, UIDataField{"vTasksEmpty", false}},
      {UIListField{"vTasks", role_names, MakeFilteredListOfTasks(app)}});
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaFilterChanged", *this,
                         EXEC(this, FilterTasks));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaTaskChosen", *this,
                         EXEC(this, HandleChosenTask));
  EXEC_NEXT(Display);
}

void ChooseTask::Display(AppData &app) {
  QList<QVariantList> tasks = MakeFilteredListOfTasks(app);
  SetUIDataField(app, kViewSlot, "vLoading", false);
  SetUIDataField(app, kViewSlot, "vTasksEmpty", execs.isEmpty());
  GetUIListField(app, kViewSlot, "vTasks").SetItems(tasks);
  EXEC_NEXT(KeepAlive);
}

void ChooseTask::FilterTasks(AppData &app) {
  filter = GetEventArg(app, 0).toString();
  Display(app);
}

void ChooseTask::HandleChosenTask(AppData &app) {
  int i = GetEventArg(app, 0).toInt();
  Q_ASSERT(i >= 0 && i < execs.size());
  result = execs[i];
}

QList<QVariantList> ChooseTask::MakeFilteredListOfTasks(AppData &app) {
  QList<QVariantList> rows;
  for (int i = 0; i < execs.size(); i++) {
    QString exec = ShortenTaskCmd(execs[i], *app.current_project);
    AppendToUIListIfMatches(rows, filter, {i, exec}, {1});
  }
  return rows;
}
