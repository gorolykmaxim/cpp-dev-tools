#include "task_execution_list_model.h"

#include "application.h"
#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[TaskExecutionListController]"

TaskExecutionListModel::TaskExecutionListModel(QObject* parent)
    : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "subTitle"}, {2, "icon"}, {3, "iconColor"}});
  searchable_roles = {0, 1};
  SetEmptyListPlaceholder("No tasks have been executed yet");
  Application& app = Application::Get();
  app.view.SetWindowTitle("Task Executions");
  QObject::connect(&app.task, &TaskSystem::executionFinished, this,
                   [this](QUuid) { Reload(); });
  QObject::connect(&app.task, &TaskSystem::selectedExecutionChanged, this,
                   [this] { emit executionChanged(); });
  QObject::connect(this, &TextListModel::selectedItemChanged, this, [this] {
    int i = GetSelectedItemIndex();
    if (i < 0) {
      return;
    }
    const TaskExecution& exec = list[i];
    Application::Get().task.SetSelectedExecutionId(exec.id);
  });
  Reload();
}

QVariantList TaskExecutionListModel::GetRow(int i) const {
  const TaskExecution& exec = list[i];
  UiIcon icon = exec.GetStatusAsIcon();
  return {exec.task_name,
          exec.start_time.toString(Application::kDateTimeFormat), icon.icon,
          icon.color};
}

int TaskExecutionListModel::GetRowCount() const { return list.size(); }

void TaskExecutionListModel::Reload() {
  LOG() << "Refreshing history of executions";
  Application& app = Application::Get();
  const Project& current_project = app.project.GetCurrentProject();
  app.task.FetchExecutions(current_project.id)
      .Then(this, [this, &app](const QList<TaskExecution>& result) {
        list = result;
        if (app.task.GetSelectedExecutionId().isNull() && !list.isEmpty()) {
          app.task.SetSelectedExecutionId(list.last().id);
        }
        int index = -1;
        QUuid id = Application::Get().task.GetSelectedExecutionId();
        for (int i = 0; i < list.size(); i++) {
          if (list[i].id == id) {
            index = i;
            break;
          }
        }
        Load(index);
      });
}

bool TaskExecutionListModel::IsSelectedExecutionRunning() const {
  Application& app = Application::Get();
  QUuid id = app.task.GetSelectedExecutionId();
  return app.task.FindExecutionById(id);
}

void TaskExecutionListModel::rerunSelectedExecution(bool repeat_until_fail) {
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  const TaskExecution& exec = list[i];
  LOG() << "Rerunning execution" << exec.id;
  Application::Get().task.RunTaskOfExecution(exec, repeat_until_fail);
}

void TaskExecutionListModel::removeFinishedExecutions() {
  QUuid project_id = Application::Get().project.GetCurrentProject().id;
  LOG() << "Clearing task execution history of project" << project_id;
  IoTask::Run(
      this,
      [project_id] {
        Database::ExecCmd("DELETE FROM task_execution WHERE project_id=?",
                          {project_id});
      },
      [this] { Reload(); });
}

void TaskExecutionListModel::openExecutionOutput() const {
  Application::Get().view.SetCurrentView("TaskExecution.qml");
}
