#include "user_command_system.h"

#include "application.h"

#define LOG() qDebug() << "[UserCommandSystem]"

QString UserCommand::GetFormattedShortcut() const {
  QString result = shortcut.toUpper();
#if __APPLE__
  result.replace("CTRL", "\u2318");
  result.replace("SHIFT", "\u21e7");
  result.replace("ALT", "\u2325");
#endif
  return result;
}

UserCommandListModel::UserCommandListModel(QObject* parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "group"}, {1, "name"}, {2, "shortcut"}, {3, "index"}});
}

QVariantList UserCommandListModel::GetRow(int i) const {
  const UserCommand& cmd = list[i];
  return {cmd.group, cmd.name, cmd.shortcut, i};
}

int UserCommandListModel::GetRowCount() const { return list.size(); }

UserCommandSystem::UserCommandSystem()
    : user_commands(new UserCommandListModel(this)) {}

void UserCommandSystem::RegisterCommands() {
  RegisterCommand("General", "Execute Command", "Ctrl+P", [] {
    Application::Get().view.DisplaySearchUserCommandDialog();
  });
  RegisterCommand("General", "Close Project", "Ctrl+W", [] {
    Application::Get().project.SetCurrentProject(Project());
  });
  RegisterCommand("Task", "Run Last", "Ctrl+R",
                  [] { Application::Get().task.ExecuteTask(0); });
  RegisterCommand("Task", "Run Last Until Fails", "Ctrl+Shift+R",
                  [] { Application::Get().task.ExecuteTask(0, true); });
  RegisterCommand("Task", "Terminate Execution", "Ctrl+Shift+T", [] {
    Application& app = Application::Get();
    app.task.CancelExecution(app.task.GetSelectedExecutionId(), false);
  });
  RegisterCommand("Task", "Kill Execution", "Ctrl+Shift+K", [] {
    Application& app = Application::Get();
    app.task.CancelExecution(app.task.GetSelectedExecutionId(), true);
  });
  RegisterCommand("Task", "Tasks", "Ctrl+T", [] {
    Application::Get().view.SetCurrentView("TaskList.qml");
  });
  RegisterCommand("Task", "Task Executions", "Ctrl+E", [] {
    Application::Get().view.SetCurrentView("TaskExecutionList.qml");
  });
  RegisterCommand("Window", "Default Size", "Ctrl+Shift+M",
                  [] { Application::Get().view.SetDefaultWindowSize(); });
  LOG() << "Comitting changes to user command list";
  user_commands->Load();
}

const QList<UserCommand>& UserCommandSystem::GetUserCommands() const {
  return user_commands->list;
}

void UserCommandSystem::ExecuteCommand(int i) {
  UserCommand& cmd = user_commands->list[i];
  LOG() << "Executing user command" << cmd.group << cmd.name;
  cmd.callback();
}

void UserCommandSystem::RegisterCommand(const QString& group,
                                        const QString& name,
                                        const QString& shortcut,
                                        const std::function<void()>& callback) {
  LOG() << "Registering user command" << group << name;
  user_commands->list.append(UserCommand{group, name, shortcut, callback});
}
