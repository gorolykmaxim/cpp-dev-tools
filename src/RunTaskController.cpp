#include "RunTaskController.hpp"

#include "Application.hpp"

RunTaskController::RunTaskController(QObject* parent) : QObject(parent) {
  Application::Get().view_controller.SetWindowTitle("Run Task");
}

void RunTaskController::ExecuteTask(const QString& command) const {
  Application& app = Application::Get();
  app.task.ExecuteTask(command);
  app.view_controller.SetCurrentView("TaskExecutionHistory.qml");
}
