#include "task_list_controller.h"

#include "application.h"

#define LOG() qDebug() << "[TaskListController]"

TaskListModel::TaskListModel(QObject *parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "subTitle"}, {2, "icon"}});
  searchable_roles = {0, 1};
  SetEmptyListPlaceholder("No tasks found");
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
  return {name, details, "code"};
}

int TaskListModel::GetRowCount() const {
  return Application::Get().task.GetTasks().size();
}

TaskListController::TaskListController(QObject *parent)
    : QObject(parent), tasks(new TaskListModel(this)) {
  Application &app = Application::Get();
  QObject::connect(&app.task, &TaskSystem::taskListRefreshed, this, [this] {
    tasks->Load();
    tasks->SetPlaceholder();
  });
  QObject::connect(tasks, &TextListModel::selectedItemChanged, this,
                   [this] { emit selectedTaskIndexChanged(); });
  app.view.SetWindowTitle("Tasks");
  tasks->SetPlaceholder("Looking for tasks...");
  app.task.FindTasks();
}

int TaskListController::GetSelectedTaskIndex() const {
  return tasks->GetSelectedItemIndex();
}
