#include "user_command_system.h"

#include "application.h"
#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[UserCommandSystem]"

static UserCommand ReadFromSql(QSqlQuery& sql) {
  UserCommand cmd;
  cmd.group = sql.value(0).toString();
  cmd.name = sql.value(1).toString();
  cmd.shortcut = sql.value(2).toString();
  return cmd;
}

QString UserCommand::GetFormattedShortcut() const {
  QString result = shortcut.toUpper();
#if __APPLE__
  result.replace("CTRL", "\u2318");
  result.replace("SHIFT", "\u21e7");
  result.replace("ALT", "\u2325");
#endif
  return result;
}

GlobalUserCommandListModel::GlobalUserCommandListModel(QObject* parent)
    : TextListModel(parent) {
  SetRoleNames({{0, "group"}, {1, "name"}, {2, "shortcut"}, {3, "index"}});
}

QVariantList GlobalUserCommandListModel::GetRow(int i) const {
  const GlobalUserCommand& cmd = list[i];
  return {cmd.group, cmd.name, cmd.shortcut, i};
}

int GlobalUserCommandListModel::GetRowCount() const { return list.size(); }

UserCommandSystem::UserCommandSystem()
    : user_commands(new GlobalUserCommandListModel(this)) {}

static void RegisterCmd(const QString& group, const QString& name,
                        const QString& shortcut, QList<Database::Cmd>& cmds,
                        GlobalUserCommandListModel* model,
                        std::function<void()>&& cb) {
  LOG() << "Registering user command" << group << name;
  model->list.append(GlobalUserCommand{{group, name, shortcut}, cb});
  cmds.append(
      Database::Cmd("INSERT OR IGNORE INTO user_command VALUES(?,?,?,?)",
                    {group, name, shortcut, true}));
}
void UserCommandSystem::Initialize() {
  QList<Database::Cmd> cmds;
  RegisterCmd("File", "Close Project", "Ctrl+W", cmds, user_commands,
              [] { Application::Get().project.SetCurrentProject(Project()); });
  RegisterCmd("File", "Settings", "Ctrl+,", cmds, user_commands,
              [] { Application::Get().view.SetCurrentView("Settings.qml"); });
  RegisterCmd("Edit", "Find In Files", "Ctrl+Shift+F", cmds, user_commands, [] {
    Application::Get().view.SetCurrentView("FindInFiles.qml");
  });
  RegisterCmd("View", "Commands", "Ctrl+P", cmds, user_commands,
              [] { Application::Get().view.DisplaySearchUserCommandDialog(); });
  RegisterCmd("View", "Tasks", "Ctrl+T", cmds, user_commands,
              [] { Application::Get().view.SetCurrentView("TaskList.qml"); });
  RegisterCmd("View", "Task Executions", "Ctrl+E", cmds, user_commands, [] {
    Application::Get().view.SetCurrentView("TaskExecutionList.qml");
  });
  RegisterCmd("View", "Notifications", "Ctrl+N", cmds, user_commands, [] {
    Application::Get().view.SetCurrentView("NotificationList.qml");
  });
  RegisterCmd("View", "Documentation", "F1", cmds, user_commands, [] {
    Application::Get().view.SetCurrentView("Documentation.qml");
  });
  RegisterCmd("View", "SQLite Databases", "Ctrl+Shift+D", cmds, user_commands,
              [] { Application::Get().view.SetCurrentView("SqliteList.qml"); });
  RegisterCmd("View", "SQLite Tables", "Ctrl+Shift+S", cmds, user_commands, [] {
    Application::Get().view.SetCurrentView("SqliteTable.qml");
  });
  RegisterCmd(
      "View", "SQLite Query Editor", "Ctrl+Shift+E", cmds, user_commands,
      [] { Application::Get().view.SetCurrentView("SqliteQueryEditor.qml"); });
  RegisterCmd("View", "Terminal", "F12", cmds, user_commands,
              [] { OsCommand::OpenTerminalInCurrentDir(); });
  RegisterCmd("Run", "Run Last Task", "Ctrl+R", cmds, user_commands, [] {
    Application& app = Application::Get();
    app.task.RunTaskOfExecution(app.task.GetLastExecution(), false,
                                "TaskExecution.qml");
  });
  RegisterCmd("Run", "Run Last Task as QtTest", "Ctrl+Y", cmds, user_commands,
              [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), false,
                                            "QtestExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task as Google Test", "Ctrl+G", cmds,
              user_commands, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), false,
                                            "GtestExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task Until Fails", "Ctrl+Shift+R", cmds,
              user_commands, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), true,
                                            "TaskExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task as QtTest Until Fails", "Ctrl+Shift+Y",
              cmds, user_commands, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), true,
                                            "QtestExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task as Google Test Until Fails", "Ctrl+Shift+G",
              cmds, user_commands, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), true,
                                            "GtestExecution.qml");
              });
  RegisterCmd("Run", "Run CMake", "Ctrl+Shift+C", cmds, user_commands,
              [] { Application::Get().view.SetCurrentView("RunCmake.qml"); });
  RegisterCmd("Run", "Terminate Task Execution", "Ctrl+Shift+T", cmds,
              user_commands,
              [] { Application::Get().task.cancelSelectedExecution(false); });
  RegisterCmd("Run", "Kill Task Execution", "Ctrl+Alt+Shift+T", cmds,
              user_commands,
              [] { Application::Get().task.cancelSelectedExecution(true); });
  RegisterCmd("Git", "Pull", "Ctrl+Shift+P", cmds, user_commands,
              [] { Application::Get().git.Pull(); });
  RegisterCmd("Git", "Push", "Ctrl+Shift+K", cmds, user_commands,
              [] { Application::Get().git.Push(); });
  RegisterCmd("Git", "Commit", "Ctrl+K", cmds, user_commands,
              [] { Application::Get().view.SetCurrentView("GitCommit.qml"); });
  RegisterCmd("Git", "Branches", "Ctrl+B", cmds, user_commands, [] {
    Application::Get().view.SetCurrentView("GitBranchList.qml");
  });
  RegisterCmd("Git", "Log", "Ctrl+L", cmds, user_commands,
              [] { Application::Get().view.SetCurrentView("GitLog.qml"); });
  RegisterCmd("Git", "File Diff", "F8", cmds, user_commands, [] {
    Application::Get().view.SetCurrentView("GitFileDiff.qml");
  });
  RegisterCmd("Window", "Default Size", "Ctrl+Shift+M", cmds, user_commands,
              [] { Application::Get().view.SetDefaultWindowSize(); });
  Database::ExecCmdsAsync(cmds);
  LOG() << "Comitting changes to user command list";
  IoTask::Run<QList<UserCommand>>(
      this,
      [] {
        return Database::ExecQueryAndRead<UserCommand>(
            "SELECT \"group\", name, shortcut FROM user_command WHERE "
            "global=true",
            ReadFromSql);
      },
      [this](QList<UserCommand> cmds) {
        for (const UserCommand& cmd : cmds) {
          for (GlobalUserCommand& gcmd : user_commands->list) {
            if (cmd.group == gcmd.group && cmd.name == gcmd.name) {
              gcmd.shortcut = cmd.shortcut;
              break;
            }
          }
        }
        user_commands->Load();
      });
}

const QList<GlobalUserCommand>& UserCommandSystem::GetUserCommands() const {
  return user_commands->list;
}

void UserCommandSystem::executeCommand(int i) {
  if (i < 0 || i >= user_commands->list.size()) {
    return;
  }
  GlobalUserCommand& cmd = user_commands->list[i];
  LOG() << "Executing user command" << cmd.group << cmd.name;
  cmd.callback();
}
