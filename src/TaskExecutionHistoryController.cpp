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
  const TaskExecution& exec = list[i];
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
  Application& app = Application::Get();
  const Project& current_project = app.project_context.GetCurrentProject();
  QString cmd = TaskExecution::ShortenCommand(exec.command, current_project);
  return {exec.id, cmd, exec.start_time.toString(), icon, iconColor, false};
}

int TaskExecutionListModel::GetRowCount() const { return list.size(); }

TaskExecutionHistoryController::TaskExecutionHistoryController(QObject* parent)
    : QObject(parent), executions(new TaskExecutionListModel(this)) {
  QObject::connect(&Application::Get().task_executor,
                   &TaskExecutor::executionFinished, this,
                   &TaskExecutionHistoryController::HandleExecutionFinished);
  LoadExecutions();
}

bool TaskExecutionHistoryController::AreExecutionsEmpty() const {
  return executions->list.size() == 0;
}

void TaskExecutionHistoryController::SelectExecution(QUuid id) {
  execution_id = id;
  emit executionChanged();
}

void TaskExecutionHistoryController::HandleExecutionFinished(QUuid id) {
  LoadExecutions();
  if (execution_id == id) {
    emit executionChanged();
  }
}

void TaskExecutionHistoryController::HandleExecutionOutputChanged(QUuid id) {
  if (execution_id == id) {
    emit executionChanged();
  }
}

void TaskExecutionHistoryController::LoadExecutions() {
  LOG() << "Refreshing history of executions";
  Application& app = Application::Get();
  const Project& current_project = app.project_context.GetCurrentProject();
  app.task_executor.FetchExecutions(this, current_project.id,
                                    [this](const QList<TaskExecution>& result) {
                                      executions->list = result;
                                      executions->Load();
                                      emit executionsChanged();
                                    });
}
