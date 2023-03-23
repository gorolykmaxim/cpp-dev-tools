#include "TaskListController.hpp"

#include "Application.hpp"
#include "Database.hpp"

#define LOG() qDebug() << "[TaskListController]"

TaskListModel::TaskListModel(QObject *parent) : QVariantListModel(parent) {
  SetRoleNames({{0, "idx"}, {1, "title"}, {2, "subTitle"}});
  searchable_roles = {1, 2};
}

QVariantList TaskListModel::GetRow(int i) const {
  const Task &task = list[i];
  QString name = TaskSystem::GetName(task);
  QString details;
  if (!task.executable.IsNull()) {
    details = task.executable.path;
  } else {
    details = task.id;
  }
  return {i, name, details};
}

int TaskListModel::GetRowCount() const { return list.size(); }

static TaskId ReadTaskId(QSqlQuery &query) { return query.value(0).toString(); }

TaskListController::TaskListController(QObject *parent)
    : QObject(parent), tasks(new TaskListModel(this)), is_loading(true) {
  Application &app = Application::Get();
  app.view.SetWindowTitle("Tasks");
  SetIsLoading(true);
  QString project_path = app.project.GetCurrentProject().path;
  app.RunIOTask<QList<Task>>(
      this,
      [project_path] {
        QList<Task> results;
        QDirIterator it(project_path, QDir::Files | QDir::Executable,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
          QString path = it.next();
          path.replace(project_path, ".");
          Task task("exec:" + path);
          task.executable.path = path;
          results.append(task);
        }
        std::sort(results.begin(), results.end(),
                  [](const Task &a, const Task &b) { return a.id < b.id; });
        // Move executed tasks to the beginning so the most recently executed
        // task is first.
        QList<TaskId> task_ids = Database::ExecQueryAndRead<TaskId>(
            "SELECT task_id, MAX(start_time) as start_time "
            "FROM task_execution "
            "GROUP BY task_id "
            "ORDER BY start_time ASC",
            ReadTaskId);
        for (const TaskId &id : task_ids) {
          int index = -1;
          for (int i = 0; i < results.size(); i++) {
            if (results[i].id == id) {
              index = i;
              break;
            }
          }
          if (index >= 0) {
            results.move(index, 0);
          }
        }
        return results;
      },
      [this](QList<Task> results) {
        tasks->list = results;
        tasks->Load();
        SetIsLoading(false);
      });
}

bool TaskListController::ShouldShowPlaceholder() const {
  return is_loading || tasks->list.isEmpty();
}

void TaskListController::ExecuteTask(int i, bool repeat_until_fail) const {
  Application &app = Application::Get();
  const Task &task = tasks->list[i];
  app.task.ExecuteTask(task, repeat_until_fail);
  app.view.SetCurrentView("TaskExecutionHistory.qml");
}

bool TaskListController::IsLoading() const { return is_loading; }

void TaskListController::SetIsLoading(bool value) {
  is_loading = value;
  emit isLoadingChanged();
}
