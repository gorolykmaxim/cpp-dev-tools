#include "UserCommandController.hpp"

#include "Application.hpp"

#define LOG() qDebug() << "[UserCommandController]"

UserCommandListModel::UserCommandListModel(QObject* parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "group"}, {1, "name"}, {2, "shortcut"}, {3, "index"}});
}

QVariantList UserCommandListModel::GetRow(int i) const {
  const UserCommand& cmd = list[i];
  return {cmd.group, cmd.name, cmd.shortcut, i};
}

int UserCommandListModel::GetRowCount() const { return list.size(); }

UserCommandController::UserCommandController()
    : user_commands(new UserCommandListModel(this)) {}

void UserCommandController::RegisterCommands() {
  RegisterCommand("General", "Execute Command", "Ctrl+P", [] {
    Application::Get().view.DisplaySearchUserCommandDialog();
  });
  RegisterCommand("General", "Close Project", "Ctrl+W", [] {
    Application::Get().project.SetCurrentProject(Project());
  });
  RegisterCommand("Task", "Run Task", "Ctrl+R", [] {
    Application::Get().view.SetCurrentView("RunTask.qml");
  });
  RegisterCommand("Task", "Task Execution History", "Ctrl+E", [] {
    Application::Get().view.SetCurrentView("TaskExecutionHistory.qml");
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
