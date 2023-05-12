#include "user_command_system.h"

#include "application.h"
#include "os_command.h"

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
  RegisterCommand("File", "Close Project", "Ctrl+W", [] {
    Application::Get().project.SetCurrentProject(Project());
  });
  RegisterCommand("File", "Settings", "Ctrl+,", [] {
    Application::Get().view.SetCurrentView("Settings.qml");
  });
  RegisterCommand("Edit", "Find In Files", "Ctrl+Shift+F", [] {
    Application::Get().view.SetCurrentView("FindInFiles.qml");
  });
  RegisterCommand("View", "Commands", "Ctrl+P", [] {
    Application::Get().view.DisplaySearchUserCommandDialog();
  });
  RegisterCommand("View", "Tasks", "Ctrl+T", [] {
    Application::Get().view.SetCurrentView("TaskList.qml");
  });
  RegisterCommand("View", "Task Executions", "Ctrl+E", [] {
    Application::Get().view.SetCurrentView("TaskExecutionList.qml");
  });
  RegisterCommand("View", "Notifications", "Ctrl+N", [] {
    Application::Get().view.SetCurrentView("NotificationList.qml");
  });
  RegisterCommand("View", "Documentation", "F1", [] {
    Application::Get().view.SetCurrentView("Documentation.qml");
  });
  RegisterCommand("View", "SQLite Databases", "Ctrl+Shift+D", [] {
    Application::Get().view.SetCurrentView("SqliteList.qml");
  });
  RegisterCommand("View", "SQLite Tables", "Ctrl+Shift+S", [] {
    Application::Get().view.SetCurrentView("SqliteTable.qml");
  });
  RegisterCommand("View", "SQLite Query Editor", "Ctrl+Shift+E", [] {
    Application::Get().view.SetCurrentView("SqliteQueryEditor.qml");
  });
  RegisterCommand("View", "Terminal", "F12",
                  [] { OsCommand::OpenTerminalInCurrentDir(); });
  RegisterCommand("Run", "Run Last Task", "Ctrl+R",
                  [] { Application::Get().task.executeTask(0); });
  RegisterCommand("Run", "Run Last Task Until Fails", "Ctrl+Shift+R",
                  [] { Application::Get().task.executeTask(0, true); });
  RegisterCommand("Run", "Terminate Task Execution", "Ctrl+Shift+T", [] {
    Application& app = Application::Get();
    app.task.cancelExecution(app.task.GetSelectedExecutionId(), false);
  });
  RegisterCommand("Run", "Kill Task Execution", "Ctrl+Shift+K", [] {
    Application& app = Application::Get();
    app.task.cancelExecution(app.task.GetSelectedExecutionId(), true);
  });
  RegisterCommand("Git", "Pull", "Ctrl+Shift+U", [] { GitSystem::Pull(); });
  RegisterCommand("Git", "Push", "Ctrl+Shift+P", [] { GitSystem::Push(); });
  RegisterCommand("Git", "Branches", "Ctrl+B", [] {
    Application::Get().view.SetCurrentView("GitBranchList.qml");
  });
  RegisterCommand("Window", "Default Size", "Ctrl+Shift+M",
                  [] { Application::Get().view.SetDefaultWindowSize(); });
  LOG() << "Comitting changes to user command list";
  user_commands->Load();
}

const QList<UserCommand>& UserCommandSystem::GetUserCommands() const {
  return user_commands->list;
}

void UserCommandSystem::executeCommand(int i) {
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
