#include "task_list_controller.h"

#include "application.h"
#include "database.h"

#define LOG() qDebug() << "[TaskListController]"

TaskListModel::TaskListModel(QObject *parent) : QVariantListModel(parent) {
  SetRoleNames({{0, "idx"}, {1, "title"}, {2, "subTitle"}});
  searchable_roles = {1, 2};
}

QVariantList TaskListModel::GetRow(int i) const {
  const Task &task = Application::Get().task.GetTasks()[i];
  QString name = TaskSystem::GetName(task);
  QString details;
  if (!task.executable.IsNull()) {
    details = task.executable.path;
  } else {
    details = task.id;
  }
  return {i, name, details};
}

int TaskListModel::GetRowCount() const {
  return Application::Get().task.GetTasks().size();
}

TaskListController::TaskListController(QObject *parent)
    : QObject(parent), tasks(new TaskListModel(this)), is_loading(true) {
  Application &app = Application::Get();
  QObject::connect(&app.task, &TaskSystem::taskListRefreshed, this, [this] {
    tasks->Load();
    SetIsLoading(false);
  });
  app.view.SetWindowTitle("Tasks");
  app.task.FindTasks();
}

bool TaskListController::ShouldShowPlaceholder() const {
  return is_loading || Application::Get().task.GetTasks().isEmpty();
}

bool TaskListController::IsLoading() const { return is_loading; }

void TaskListController::SetIsLoading(bool value) {
  is_loading = value;
  emit isLoadingChanged();
}