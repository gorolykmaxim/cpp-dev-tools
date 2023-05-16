#include "task_execution_list_controller.h"

#include "application.h"
#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[TaskExecutionListController]"

TaskExecutionListModel::TaskExecutionListModel(QObject* parent)
    : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "subTitle"}, {2, "icon"}, {3, "iconColor"}});
  searchable_roles = {0, 1};
  SetEmptyListPlaceholder("No tasks have been executed yet");
}

QVariantList TaskExecutionListModel::GetRow(int i) const {
  const TaskExecution& exec = list[i];
  UiIcon icon = TaskSystem::GetStatusAsIcon(exec);
  return {exec.task_name,
          exec.start_time.toString(Application::kDateTimeFormat), icon.icon,
          icon.color};
}

int TaskExecutionListModel::GetRowCount() const { return list.size(); }

const TaskExecution* TaskExecutionListModel::GetToBeSelected() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? nullptr : &list[i];
}

int TaskExecutionListModel::IndexOfCurrentlySelected() const {
  QUuid id = Application::Get().task.GetSelectedExecutionId();
  for (int i = 0; i < list.size(); i++) {
    if (list[i].id == id) {
      return i;
    }
  }
  return -1;
}

TaskExecutionListController::TaskExecutionListController(QObject* parent)
    : QObject(parent), executions(new TaskExecutionListModel(this)) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Task Executions");
  QObject::connect(&app.task, &TaskSystem::executionFinished, this,
                   [this](QUuid) { LoadExecutions(); });
  QObject::connect(&app.task, &TaskSystem::selectedExecutionChanged, this,
                   [this] { emit executionChanged(); });
  QObject::connect(
      executions, &TextListModel::selectedItemChanged, this, [this] {
        if (const TaskExecution* exec = executions->GetToBeSelected()) {
          Application::Get().task.SetSelectedExecutionId(exec->id);
        }
      });
  LoadExecutions();
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
        executions->Load(executions->IndexOfCurrentlySelected());
      });
}

int TaskExecutionListController::IndexOfExecutionTask() const {
  Application& app = Application::Get();
  QUuid execution_id = app.task.GetSelectedExecutionId();
  const QList<Task>& tasks = app.task.GetTasks();
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
