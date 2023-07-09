#include "user_command_system.h"

#include "application.h"
#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[UserCommandSystem]"

static const QString kSelectGlobalCmdsSql =
    "SELECT \"group\", name, shortcut FROM user_command WHERE global=true";
static const QString kSelectLocalCmdsSql =
    "SELECT \"group\", name, shortcut FROM user_command WHERE global=false";

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

UserCommand UserCommandSystem::ReadUserCommandFromSql(QSqlQuery& sql) {
  UserCommand cmd;
  cmd.group = sql.value(0).toString();
  cmd.name = sql.value(1).toString();
  cmd.shortcut = sql.value(2).toString();
  return cmd;
}

static void RegisterCmd(const QString& group, const QString& name,
                        const QString& shortcut, QList<Database::Cmd>& cmds,
                        GlobalUserCommandListModel* model,
                        UserCommandIndex& default_cmd_index,
                        std::function<void()>&& cb) {
  LOG() << "Registering user command" << group << name;
  model->list.append(GlobalUserCommand{{group, name, shortcut}, cb});
  cmds.append(
      Database::Cmd("INSERT OR IGNORE INTO user_command VALUES(?,?,?,?)",
                    {group, name, shortcut, true}));
  default_cmd_index[group][name] = shortcut;
}

static void RegisterLocalCmd(const QString& group, const QString& name,
                             const QString& shortcut,
                             QList<Database::Cmd>& cmds,
                             UserCommandIndex& index) {
  LOG() << "Registering context menu user command" << group << name;
  index[group][name] = shortcut;
  cmds.append(
      Database::Cmd("INSERT OR IGNORE INTO user_command VALUES(?,?,?,?)",
                    {group, name, shortcut, false}));
}

void UserCommandSystem::Initialize() {
  QList<Database::Cmd> cmds;
  RegisterCmd("File", "Close Project", "Ctrl+W", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().project.SetCurrentProject(Project()); });
  RegisterCmd("File", "Settings", "Ctrl+,", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().view.SetCurrentView("Settings.qml"); });
  RegisterCmd("Edit", "Find In Files", "Ctrl+Shift+F", cmds, user_commands,
              default_user_cmd_index, [] {
                Application::Get().view.SetCurrentView("FindInFiles.qml");
              });
  RegisterCmd("View", "Commands", "Ctrl+P", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().view.DisplaySearchUserCommandDialog(); });
  RegisterCmd("View", "Tasks", "Ctrl+T", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().view.SetCurrentView("TaskList.qml"); });
  RegisterCmd("View", "Task Executions", "Ctrl+E", cmds, user_commands,
              default_user_cmd_index, [] {
                Application::Get().view.SetCurrentView("TaskExecutionList.qml");
              });
  RegisterCmd("View", "Notifications", "Ctrl+N", cmds, user_commands,
              default_user_cmd_index, [] {
                Application::Get().view.SetCurrentView("NotificationList.qml");
              });
  RegisterCmd("View", "Documentation", "F1", cmds, user_commands,
              default_user_cmd_index, [] {
                Application::Get().view.SetCurrentView("Documentation.qml");
              });
  RegisterCmd("View", "SQLite Databases", "Ctrl+Shift+D", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().view.SetCurrentView("SqliteList.qml"); });
  RegisterCmd("View", "SQLite Tables", "Ctrl+Shift+S", cmds, user_commands,
              default_user_cmd_index, [] {
                Application::Get().view.SetCurrentView("SqliteTable.qml");
              });
  RegisterCmd("View", "SQLite Query Editor", "Ctrl+Shift+E", cmds,
              user_commands, default_user_cmd_index, [] {
                Application::Get().view.SetCurrentView("SqliteQueryEditor.qml");
              });
  RegisterCmd("View", "Terminal", "F12", cmds, user_commands,
              default_user_cmd_index,
              [] { OsCommand::OpenTerminalInCurrentDir(); });
  RegisterCmd("Run", "Run Last Task", "Ctrl+R", cmds, user_commands,
              default_user_cmd_index, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), false,
                                            "TaskExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task as QtTest", "Ctrl+U", cmds, user_commands,
              default_user_cmd_index, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), false,
                                            "QtestExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task as Google Test", "Ctrl+G", cmds,
              user_commands, default_user_cmd_index, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), false,
                                            "GtestExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task Until Fails", "Ctrl+Shift+R", cmds,
              user_commands, default_user_cmd_index, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), true,
                                            "TaskExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task as QtTest Until Fails", "Ctrl+Shift+U",
              cmds, user_commands, default_user_cmd_index, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), true,
                                            "QtestExecution.qml");
              });
  RegisterCmd("Run", "Run Last Task as Google Test Until Fails", "Ctrl+Shift+G",
              cmds, user_commands, default_user_cmd_index, [] {
                Application& app = Application::Get();
                app.task.RunTaskOfExecution(app.task.GetLastExecution(), true,
                                            "GtestExecution.qml");
              });
  RegisterCmd("Run", "Run CMake", "Ctrl+Shift+C", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().view.SetCurrentView("RunCmake.qml"); });
  RegisterCmd("Run", "Terminate Task Execution", "Ctrl+Shift+T", cmds,
              user_commands, default_user_cmd_index,
              [] { Application::Get().task.cancelSelectedExecution(false); });
  RegisterCmd("Run", "Kill Task Execution", "Ctrl+Alt+Shift+T", cmds,
              user_commands, default_user_cmd_index,
              [] { Application::Get().task.cancelSelectedExecution(true); });
  RegisterCmd("Git", "Fetch", "Ctrl+Shift+O", cmds, user_commands,
              default_user_cmd_index, [] { Application::Get().git.Fetch(); });
  RegisterCmd("Git", "Pull", "Ctrl+Shift+P", cmds, user_commands,
              default_user_cmd_index, [] { Application::Get().git.Pull(); });
  RegisterCmd("Git", "Push", "Ctrl+Shift+K", cmds, user_commands,
              default_user_cmd_index, [] { Application::Get().git.Push(); });
  RegisterCmd("Git", "Commit", "Ctrl+K", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().view.SetCurrentView("GitCommit.qml"); });
  RegisterCmd(
      "Git", "Branches", "Ctrl+B", cmds, user_commands, default_user_cmd_index,
      [] { Application::Get().view.SetCurrentView("GitBranchList.qml"); });
  RegisterCmd("Git", "Log", "Ctrl+L", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().view.SetCurrentView("GitLog.qml"); });
  RegisterCmd(
      "Git", "File Diff", "F8", cmds, user_commands, default_user_cmd_index,
      [] { Application::Get().view.SetCurrentView("GitFileDiff.qml"); });
  RegisterCmd("Window", "Default Size", "Ctrl+Shift+M", cmds, user_commands,
              default_user_cmd_index,
              [] { Application::Get().view.SetDefaultWindowSize(); });
  Database::ExecCmdsAsync(cmds);
  LOG() << "Loading global user command shortcuts";
  QList<UserCommand> user_cmds = Database::ExecQueryAndReadSync<UserCommand>(
      kSelectGlobalCmdsSql, &UserCommandSystem::ReadUserCommandFromSql);
  UpdateGlobalShortcuts(user_cmds);
  LOG() << "Committing global user command list";
  user_commands->Load();
  cmds.clear();
  RegisterLocalCmd("TaskList", "Run as QtTest", "Alt+U", cmds, user_cmd_index);
  RegisterLocalCmd("TaskList", "Run as QtTest With Filter", "Ctrl+Alt+U", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskList", "Run as Google Test", "Alt+G", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskList", "Run as Google Test With Filter", "Ctrl+Alt+G",
                   cmds, user_cmd_index);
  RegisterLocalCmd("TaskList", "Run Until Fails", "Alt+Shift+R", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskList", "Run as QtTest Until Fails", "Alt+Shift+U", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskList", "Run as QtTest With Filter Until Fails",
                   "Ctrl+Alt+Shift+U", cmds, user_cmd_index);
  RegisterLocalCmd("TaskList", "Run as Google Test Until Fails", "Alt+Shift+G",
                   cmds, user_cmd_index);
  RegisterLocalCmd("TaskList", "Run as Google Test With Filter Until Fails",
                   "Ctrl+Alt+Shift+G", cmds, user_cmd_index);
  RegisterLocalCmd("FolderList", "Remove Folder", "Alt+Shift+D", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Open as QtTest", "Alt+U", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Open as Google Test", "Alt+G", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Re-Run", "Alt+Shift+R", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Re-Run as QtTest", "Alt+Shift+U", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Re-Run as Google Test", "Alt+Shift+G",
                   cmds, user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Re-Run Until Fails",
                   "Ctrl+Alt+Shift+R", cmds, user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Re-Run as QtTest Until Fails",
                   "Ctrl+Alt+Shift+U", cmds, user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Re-Run as Google Test Until Fails",
                   "Ctrl+Alt+Shift+G", cmds, user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Terminate", "Alt+Shift+T", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Kill", "Alt+Shift+K", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TaskExecutionList", "Remove Finished Executions",
                   "Alt+Shift+D", cmds, user_cmd_index);
  RegisterLocalCmd("SqliteList", "Change Path", "Alt+E", cmds, user_cmd_index);
  RegisterLocalCmd("SqliteList", "Remove From List", "Alt+Shift+D", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TableView", "Copy Cell Value", "Ctrl+C", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TableView", "Inspect Cell Value", "Ctrl+I", cmds,
                   user_cmd_index);
  RegisterLocalCmd("NotificationList", "Clear Notifications", "Alt+Shift+D",
                   cmds, user_cmd_index);
  RegisterLocalCmd("Settings", "Move Up", "Shift+Up", cmds, user_cmd_index);
  RegisterLocalCmd("Settings", "Move Down", "Shift+Down", cmds, user_cmd_index);
  RegisterLocalCmd("GitDiff", "Open Chunk In Editor", "Ctrl+O", cmds,
                   user_cmd_index);
  RegisterLocalCmd("GitDiff", "Toggle Unified Diff", "Alt+U", cmds,
                   user_cmd_index);
  RegisterLocalCmd("GitCommit", "Reset", "Ctrl+Alt+Z", cmds, user_cmd_index);
  RegisterLocalCmd("GitCommit", "Rollback Chunk", "Ctrl+Alt+Z", cmds,
                   user_cmd_index);
  RegisterLocalCmd("GitBranchList", "New Branch", "Alt+N", cmds,
                   user_cmd_index);
  RegisterLocalCmd("GitBranchList", "Merge Into Current", "Alt+M", cmds,
                   user_cmd_index);
  RegisterLocalCmd("GitBranchList", "Delete", "Alt+D", cmds, user_cmd_index);
  RegisterLocalCmd("GitBranchList", "Force Delete", "Alt+Shift+D", cmds,
                   user_cmd_index);
  RegisterLocalCmd("SelectProject", "Change Path", "Alt+E", cmds,
                   user_cmd_index);
  RegisterLocalCmd("SelectProject", "Remove From List", "Alt+Shift+D", cmds,
                   user_cmd_index);
  RegisterLocalCmd("GitLog", "Checkout", "Alt+C", cmds, user_cmd_index);
  RegisterLocalCmd("GitLog", "Cherry-Pick", "Alt+P", cmds, user_cmd_index);
  RegisterLocalCmd("GitLog", "New Branch", "Alt+N", cmds, user_cmd_index);
  RegisterLocalCmd("TestExecution", "Re-Run", "Alt+Shift+R", cmds,
                   user_cmd_index);
  RegisterLocalCmd("TestExecution", "Re-Run Until Fails", "Ctrl+Alt+Shift+R",
                   cmds, user_cmd_index);
  RegisterLocalCmd("FileLinkLookup", "Open File In Editor", "Ctrl+O", cmds,
                   user_cmd_index);
  RegisterLocalCmd("FileLinkLookup", "Previous File Link", "Ctrl+Alt+Up", cmds,
                   user_cmd_index);
  RegisterLocalCmd("FileLinkLookup", "Next File Link", "Ctrl+Alt+Down", cmds,
                   user_cmd_index);
  RegisterLocalCmd("KeyboardShortcuts", "Reset Changed", "Alt+R", cmds,
                   user_cmd_index);
  RegisterLocalCmd("KeyboardShortcuts", "Reset All Changes", "Alt+Shift+R",
                   cmds, user_cmd_index);
  RegisterLocalCmd("KeyboardShortcuts", "Restore Default", "Alt+D", cmds,
                   user_cmd_index);
  RegisterLocalCmd("KeyboardShortcuts", "Restore All Defaults", "Alt+Shift+D",
                   cmds, user_cmd_index);
  for (const QString& group : user_cmd_index.keys()) {
    default_user_cmd_index[group] = user_cmd_index[group];
  }
  Database::ExecCmdsAsync(cmds);
  LOG() << "Loading context user command shortcuts";
  user_cmds = Database::ExecQueryAndReadSync<UserCommand>(
      kSelectLocalCmdsSql, &UserCommandSystem::ReadUserCommandFromSql);
  UpdateLocalShortcuts(user_cmds);
}

const QList<GlobalUserCommand>& UserCommandSystem::GetUserCommands() const {
  return user_commands->list;
}

void UserCommandSystem::ReloadCommands() {
  IoTask::Run<QList<UserCommand>>(
      this,
      [] {
        return Database::ExecQueryAndRead<UserCommand>(
            kSelectGlobalCmdsSql, &UserCommandSystem::ReadUserCommandFromSql);
      },
      [this](QList<UserCommand> cmds) { UpdateGlobalShortcuts(cmds); });
  IoTask::Run<QList<UserCommand>>(
      this,
      [] {
        return Database::ExecQueryAndRead<UserCommand>(
            kSelectLocalCmdsSql, &UserCommandSystem::ReadUserCommandFromSql);
      },
      [this](QList<UserCommand> cmds) { UpdateLocalShortcuts(cmds); });
}

QString UserCommandSystem::GetDefaultShortcut(const QString& group,
                                              const QString& name) const {
  return default_user_cmd_index[group][name];
}

void UserCommandSystem::executeCommand(int i) {
  if (i < 0 || i >= user_commands->list.size()) {
    return;
  }
  GlobalUserCommand& cmd = user_commands->list[i];
  LOG() << "Executing user command" << cmd.group << cmd.name;
  cmd.callback();
}

QString UserCommandSystem::getShortcut(const QString& group,
                                       const QString& name) const {
  return user_cmd_index[group][name];
}

void UserCommandSystem::UpdateGlobalShortcuts(const QList<UserCommand>& cmds) {
  for (const UserCommand& cmd : cmds) {
    for (GlobalUserCommand& gcmd : user_commands->list) {
      if (cmd.group == gcmd.group && cmd.name == gcmd.name) {
        gcmd.shortcut = cmd.shortcut;
        break;
      }
    }
  }
  user_commands->Load();
}

void UserCommandSystem::UpdateLocalShortcuts(const QList<UserCommand>& cmds) {
  for (const UserCommand& cmd : cmds) {
    user_cmd_index[cmd.group][cmd.name] = cmd.shortcut;
  }
}
