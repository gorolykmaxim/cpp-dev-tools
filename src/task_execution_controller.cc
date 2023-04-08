#include "task_execution_controller.h"

#include "application.h"

#define LOG() qDebug() << "[TaskExecutionController]"

TaskExecutionOutputHighlighter::TaskExecutionOutputHighlighter()
    : QSyntaxHighlighter((QObject*)nullptr) {
  error_line_format.setForeground(QBrush(Qt::red));
}

void TaskExecutionOutputHighlighter::highlightBlock(const QString& text) {
  if (stderr_line_indices.contains(currentBlock().firstLineNumber())) {
    setFormat(0, text.size(), error_line_format);
  }
}

TaskExecutionController::TaskExecutionController(QObject* parent)
    : QObject(parent) {
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
  app.task.FetchExecution(
      this, id, include_output,
      [this, include_output](const TaskExecution& exec) {
        execution_name = exec.task_name;
        execution_status = "Running...";
        if (exec.exit_code) {
          execution_status = "Exit Code: " + QString::number(*exec.exit_code);
        }
        if (include_output) {
          execution_output = exec.output;
          execution_output_highlighter.stderr_line_indices =
              exec.stderr_line_indices;
        }
        emit executionChanged();
      });
}

void TaskExecutionController::AttachTaskExecutionOutputHighlighter(
    QQuickTextDocument* document) {
  execution_output_highlighter.setDocument(document->textDocument());
}
