#include "TaskExecutionHistoryController.hpp"

#include "Application.hpp"

#define LOG() qDebug() << "[TaskExecutionHistoryController]"

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
  const TaskExecution& exec = app.task_executor.GetExecutions()[i];
  QString icon, iconColor;
  if (!exec.exit_code) {
    icon = "autorenew";
    iconColor = "green";
  } else {
    if (*exec.exit_code == 0) {
      icon = "check";
      iconColor = "";
    } else {
      icon = "error";
      iconColor = "red";
    }
  }
  Project current_project = app.project_context.GetCurrentProject();
  QString cmd = TaskExecution::ShortenTaskCmd(exec.command, current_project);
  return {exec.id, cmd, exec.start_time.toString(), icon, iconColor, false};
}

int TaskExecutionListModel::GetRowCount() const {
  return Application::Get().task_executor.GetExecutions().size();
}

TaskExecutionHistoryController::TaskExecutionHistoryController(QObject* parent)
    : QObject(parent), executions(new TaskExecutionListModel(this)) {
  TaskExecutor& task_executor = Application::Get().task_executor;
  QObject::connect(&task_executor, &TaskExecutor::executionFinished, this,
                   &TaskExecutionHistoryController::HandleExecutionFinished);
  executions->Load();
}

bool TaskExecutionHistoryController::AreExecutionsEmpty() const {
  return executions->GetRowCount() == 0;
}

void TaskExecutionHistoryController::SelectExecution(QUuid id) {
  execution_id = id;
  emit executionChanged();
}

void TaskExecutionHistoryController::HandleExecutionFinished(QUuid id) {
  executions->Load();
  emit executionsChanged();
  if (execution_id == id) {
    emit executionChanged();
  }
}

void TaskExecutionHistoryController::HandleExecutionOutputChanged(QUuid id) {
  if (execution_id == id) {
    emit executionChanged();
  }
}
