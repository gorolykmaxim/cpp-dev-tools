#include "UserCommandController.hpp"

#include "Application.hpp"
#include "UserCommandListModel.hpp"

#define LOG() qDebug() << "[UserCommandController]"

UserCommandController::UserCommandController()
    : user_commands(new UserCommandListModel(this)) {}

void UserCommandController::RegisterCommands() {
  RegisterCommand("General", "Execute Command", "Ctrl+P", [] {
    Application::Get().view_controller.DisplaySearchUserCommandDialog();
  });
  RegisterCommand("General", "Close Project", "Ctrl+W", [] {
    Application::Get().project_context.SetCurrentProject(Project());
    Application::Get().view_controller.SetCurrentView("SelectProject.qml");
  });
  RegisterCommand("Task", "Run Task", "Ctrl+R", [] {
    Application::Get().view_controller.SetCurrentView("RunTask.qml");
  });
  LOG() << "Comitting changes to user command list";
  user_commands->Load();
}

const QList<UserCommand>& UserCommandController::GetUserCommands() const {
  return user_commands->list;
}

void UserCommandController::ExecuteCommand(int i) {
  UserCommand& cmd = user_commands->list[i];
  LOG() << "Executing user command" << cmd.group << cmd.name;
  cmd.callback();
}

void UserCommandController::RegisterCommand(
    const QString& group, const QString& name, const QString& shortcut,
    const std::function<void()>& callback) {
  LOG() << "Registering user command" << group << name;
  user_commands->list.append(UserCommand{group, name, shortcut, callback});
}
