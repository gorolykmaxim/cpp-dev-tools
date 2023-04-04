#include "TaskExecutionHistoryController.hpp"

#include "Application.hpp"
#include "Database.hpp"

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
  bool is_selected = selected_execution_id == exec.id;
  return {exec.id, exec.task_name, exec.start_time.toString(),
          icon,    iconColor,      is_selected};
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
  app.view.SetWindowTitle("Task Executions");
  QObject::connect(&app.task, &TaskSystem::executionFinished, this,
                   &TaskExecutionHistoryController::HandleExecutionFinished);
  QObject::connect(
      &app.task, &TaskSystem::executionOutputChanged, this,
      &TaskExecutionHistoryController::HandleExecutionOutputChanged);
  LoadExecutions(true);
}

bool TaskExecutionHistoryController::AreExecutionsEmpty() const {
  return executions->list.size() == 0;
}

bool TaskExecutionHistoryController::IsSelectedExecutionRunning() const {
  return Application::Get().task.IsExecutionRunning(execution_id);
}

void TaskExecutionHistoryController::SelectExecution(QUuid id) {
  if (execution_id == id) {
    return;
  }
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

void TaskExecutionHistoryController::RemoveFinishedExecutions() {
  Application& app = Application::Get();
  QUuid project_id = app.project.GetCurrentProject().id;
  LOG() << "Clearing task execution history of project" << project_id;
  app.RunIOTask(
      this,
      [project_id] {
        Database::ExecCmd("DELETE FROM task_execution WHERE project_id=?",
                          {project_id});
      },
      [this] { LoadExecutions(false); });
}

void TaskExecutionHistoryController::LoadExecutions(
    bool update_selected_execution) {
  LOG() << "Refreshing history of executions";
  Application& app = Application::Get();
  const Project& current_project = app.project.GetCurrentProject();
  app.task.FetchExecutions(
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

static QString ToRichText(const TaskExecutionOutput& exec_output) {
  static const QString kStart = "<span style=\"color:red;\">";
  static const QString kEnd = "</span>";
  static const QString kLineBreak = "<br>";
  QString source = exec_output.output.toHtmlEscaped();
  QString result;
  result.reserve(source.size() * 1.5);
  int pos = 0;
  int line = 0;
  while (true) {
    int i = source.indexOf('\n', pos);
    if (i < 0) {
      break;
    }
    bool is_stderr = exec_output.stderr_line_indices.contains(line);
    if (is_stderr) {
      result += kStart;
    }
    result += source.sliced(pos, i - pos);
    if (is_stderr) {
      result += kEnd;
    }
    result += kLineBreak;
    pos = i + 1;
    line++;
  }
  return result;
}

void TaskExecutionHistoryController::LoadSelectedExecutionOutput() {
  LOG() << "Reloading selected execution's output";
  Application::Get().task.FetchExecutionOutput(
      this, execution_id, [this](const TaskExecutionOutput& exec_output) {
        execution_output = ToRichText(exec_output);
        emit executionChanged();
      });
}

void TaskExecutionHistoryController::DisplaySelectedExecution() {
  const TaskExecution* exec = executions->FindById(execution_id);
  if (!exec) {
    return;
  }
  LOG() << "Updating selected execution's status";
  execution_name = exec->task_name;
  execution_status = "Running...";
  if (exec->exit_code) {
    execution_status = "Exit Code: " + QString::number(*exec->exit_code);
  }
  emit executionChanged();
}

int TaskExecutionHistoryController::IndexOfExecutionTask() const {
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
