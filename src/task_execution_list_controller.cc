#include "task_execution_list_controller.h"

#include "application.h"
#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[TaskExecutionListController]"

TaskExecutionListModel::TaskExecutionListModel(QObject* parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "id"},
                {1, "title"},
                {2, "subTitle"},
                {3, "icon"},
                {4, "iconColor"},
                {5, "isSelected"}});
  searchable_roles = {1, 2};
}

QVariantList TaskExecutionListModel::GetRow(int i) const {
  Application& app = Application::Get();
  const TaskExecution& exec = list[i];
  UiIcon icon = TaskSystem::GetStatusAsIcon(exec);
  bool is_selected = app.task.GetSelectedExecutionId() == exec.id;
  return {exec.id,
          exec.task_name,
          exec.start_time.toString(Application::kDateTimeFormat),
          icon.icon,
          icon.color,
          is_selected};
}

int TaskExecutionListModel::GetRowCount() const { return list.size(); }

TaskExecutionListController::TaskExecutionListController(QObject* parent)
    : QObject(parent), executions(new TaskExecutionListModel(this)) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Task Executions");
  QObject::connect(&app.task, &TaskSystem::executionFinished, this,
                   [this](QUuid) { LoadExecutions(); });
  QObject::connect(&app.task, &TaskSystem::selectedExecutionChanged, this,
                   [this] { emit executionChanged(); });
  LoadExecutions();
}

bool TaskExecutionListController::AreExecutionsEmpty() const {
  return executions->list.size() == 0;
}

bool TaskExecutionListController::IsSelectedExecutionRunning() const {
  Application& app = Application::Get();
  QUuid id = app.task.GetSelectedExecutionId();
  return app.task.IsExecutionRunning(id);
}

void TaskExecutionListController::removeFinishedExecutions() {
  QUuid project_id = Application::Get().project.GetCurrentProject().id;
  LOG() << "Clearing task execution history of project" << project_id;
  IoTask::Run(
      this,
      [project_id] {
        Database::ExecCmd("DELETE FROM task_execution WHERE project_id=?",
                          {project_id});
      },
      [this] { LoadExecutions(); });
}

void TaskExecutionListController::LoadExecutions() {
  LOG() << "Refreshing history of executions";
  Application& app = Application::Get();
  const Project& current_project = app.project.GetCurrentProject();
  app.task.FetchExecutions(
      this, current_project.id,
      [this, &app](const QList<TaskExecution>& result) {
        executions->list = result;
        if (app.task.GetSelectedExecutionId().isNull() &&
            !executions->list.isEmpty()) {
          app.task.SetSelectedExecutionId(executions->list.last().id);
        }
        executions->Load();
        emit executionsChanged();
      });
}

int TaskExecutionListController::IndexOfExecutionTask() const {
  QUuid execution_id = Application::Get().task.GetSelectedExecutionId();
  const QList<Task>& tasks = Application::Get().task.GetTasks();
  for (const TaskExecution& exec : executions->list) {
    if (exec.id != execution_id) {
      continue;
    }
    for (int i = 0; i < tasks.size(); i++) {
      if (exec.task_id == tasks[i].id) {
        return i;
      }
    }
    // Didn't find the task for the corresponding execution
    return -1;
  }
  return -1;
}

void TaskExecutionListController::openExecutionOutput() const {
  Application::Get().view.SetCurrentView("TaskExecution.qml");
}
