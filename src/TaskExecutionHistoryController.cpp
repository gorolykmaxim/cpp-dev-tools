#include "TaskExecutionHistoryController.hpp"

#include "Application.hpp"

#define LOG() qDebug() << "[TaskExecutionHistoryController]"

TaskExecutionListModel::TaskExecutionListModel(QObject* parent,
                                               QUuid& selected_execution_id)
    : QVariantListModel(parent), selected_execution_id(selected_execution_id) {
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
  const Project& current_project = app.project.GetCurrentProject();
  QString cmd = TaskExecution::ShortenCommand(exec.command, current_project);
  bool is_selected = selected_execution_id == exec.id;
  return {exec.id, cmd,       exec.start_time.toString(),
          icon,    iconColor, is_selected};
}

int TaskExecutionListModel::GetRowCount() const { return list.size(); }

const TaskExecution* TaskExecutionListModel::FindById(QUuid id) const {
  const TaskExecution* result = nullptr;
  for (const TaskExecution& exec : list) {
    if (exec.id == id) {
      result = &exec;
      break;
    }
  }
  return result;
}

TaskExecutionHistoryController::TaskExecutionHistoryController(QObject* parent)
    : QObject(parent),
      executions(new TaskExecutionListModel(this, execution_id)) {
  Application& app = Application::Get();
  app.view_controller.SetWindowTitle("Task Execution History");
  QObject::connect(&app.task_executor, &TaskExecutor::executionFinished, this,
                   &TaskExecutionHistoryController::HandleExecutionFinished);
  QObject::connect(
      &app.task_executor, &TaskExecutor::executionOutputChanged, this,
      &TaskExecutionHistoryController::HandleExecutionOutputChanged);
  LoadExecutions(true);
}

bool TaskExecutionHistoryController::AreExecutionsEmpty() const {
  return executions->list.size() == 0;
}

void TaskExecutionHistoryController::SelectExecution(QUuid id) {
  execution_id = id;
  LOG() << "Execution selected:" << execution_id;
  DisplaySelectedExecution();
  LoadSelectedExecutionOutput();
}

void TaskExecutionHistoryController::HandleExecutionFinished(QUuid id) {
  LoadExecutions(execution_id == id);
}

void TaskExecutionHistoryController::HandleExecutionOutputChanged(QUuid id) {
  if (execution_id == id) {
    LoadSelectedExecutionOutput();
  }
}

void TaskExecutionHistoryController::LoadExecutions(
    bool update_selected_execution) {
  LOG() << "Refreshing history of executions";
  Application& app = Application::Get();
  const Project& current_project = app.project.GetCurrentProject();
  app.task_executor.FetchExecutions(
      this, current_project.id,
      [this, update_selected_execution](const QList<TaskExecution>& result) {
        executions->list = result;
        if (update_selected_execution) {
          if (execution_id.isNull() && !executions->list.isEmpty()) {
            execution_id = executions->list.last().id;
          }
          DisplaySelectedExecution();
          LoadSelectedExecutionOutput();
        }
        executions->Load();
        emit executionsChanged();
      });
}

void TaskExecutionHistoryController::LoadSelectedExecutionOutput() {
  LOG() << "Reloading selected execution's output";
  Application::Get().task_executor.FetchExecutionOutput(
      this, execution_id, [this](const QString& output) {
        execution_output = output.toHtmlEscaped();
        execution_output.replace("\n", "<br>");
        emit executionChanged();
      });
}

void TaskExecutionHistoryController::DisplaySelectedExecution() {
  const TaskExecution* exec = executions->FindById(execution_id);
  if (!exec) {
    return;
  }
  LOG() << "Updaing selected execution's status";
  Application& app = Application::Get();
  const Project& current_project = app.project.GetCurrentProject();
  execution_command =
      TaskExecution::ShortenCommand(exec->command, current_project);
  execution_status = "Running...";
  if (exec->exit_code) {
    execution_status = "Exit Code: " + QString::number(*exec->exit_code);
  }
  emit executionChanged();
}
