#include "task_execution_controller.h"

#include "application.h"

#define LOG() qDebug() << "[TaskExecutionController]"

TaskExecutionController::TaskExecutionController(QObject* parent)
    : QObject(parent),
      execution_formatter(new TaskExecutionOutputFormatter(this)) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Task Execution Output");
  QObject::connect(&app.task, &TaskSystem::executionOutputChanged, this,
                   [this, &app](QUuid id) {
                     if (app.task.GetSelectedExecutionId() == id) {
                       LoadExecution(true);
                     }
                   });
  QObject::connect(&app.task, &TaskSystem::executionFinished, this,
                   [this, &app](QUuid id) {
                     if (app.task.GetSelectedExecutionId() == id) {
                       LoadExecution(false);
                     }
                   });
  LoadExecution(true);
}

void TaskExecutionController::LoadExecution(bool include_output) {
  LOG() << "Reloading selected execution including output:" << include_output;
  Application& app = Application::Get();
  QUuid id = app.task.GetSelectedExecutionId();
  app.task.FetchExecution(id, include_output)
      .Then(this, [this, include_output](const TaskExecution& exec) {
        execution_name = exec.task_name;
        execution_status = "<b>Running...</b>";
        if (exec.exit_code) {
          execution_status =
              "Exit Code: <b>" + QString::number(*exec.exit_code) + "</b>";
        }
        execution_status +=
            "&nbsp;&nbsp;&nbsp;Start Time: <b>" +
            exec.start_time.toString(Application::kDateTimeFormat) + "</b>";
        UiIcon icon = exec.GetStatusAsIcon();
        execution_icon = icon.icon;
        execution_icon_color = icon.color;
        if (include_output) {
          execution_output = exec.output;
          execution_formatter->stderr_line_indicies = exec.stderr_line_indices;
        }
        emit executionChanged();
      });
}

TaskExecutionOutputFormatter::TaskExecutionOutputFormatter(QObject* parent)
    : TextFormatter(parent) {
  error_line_format.setForeground(QBrush(Qt::red));
}

QList<TextFormat> TaskExecutionOutputFormatter::Format(const QString& text,
                                                       LineInfo line) const {
  QList<TextFormat> results;
  if (stderr_line_indicies.contains(line.number)) {
    TextFormat f;
    f.offset = 0;
    f.length = text.size();
    f.format = error_line_format;
    results.append(f);
  }
  return results;
}
