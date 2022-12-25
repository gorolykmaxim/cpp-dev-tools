#include "AppData.hpp"
#include "UI.hpp"
#include "Process.hpp"
#include "SearchUserCmds.hpp"
#include "CloseProject.hpp"
#include "ExecTasks.hpp"

QString& Profile::operator[](const QString& key) {
  return variables[key];
}

QString Profile::operator[](const QString& key) const {
  return variables[key];
}

QString Profile::GetName() const {
  return variables["name"];
}

QList<QString> Profile::GetVariableNames() const {
  return variables.keys();
}

bool Profile::Contains(const QString& key) const {
  return variables.contains(key);
}

Project::Project(const QString& path) : path(path), profile(-1) {}

QString Project::GetPathRelativeToHome() const {
  QString home_str = QStandardPaths::writableLocation(
      QStandardPaths::HomeLocation);
  if (path.startsWith(home_str)) {
    return "~/" + QDir(home_str).relativeFilePath(path);
  } else {
    return path;
  }
}

QString Project::GetFolderName() const {
  QDir dir(path);
  dir.cdUp();
  return dir.dirName();
}

bool Project::operator==(const Project& project) const {
  return path == project.path;
}

bool Project::operator!=(const Project& project) const {
  return !(*this == project);
}

QString UserCmd::GetFormattedShortcut() const {
  QString result = shortcut.toUpper();
#if __APPLE__
  result.replace("CTRL", "\u2318");
#endif
  return result;
}

template<typename P, typename... Args>
static void RegisterUserCmd(AppData* app, const QString& event_type,
                            const QString& group, const QString& name,
                            const QString& shortcut,
                            const QString& process_name,
                            bool cancel_other_named_processes,
                            Args&&... args) {
  UserCmd& cmd = app->user_cmds[event_type];
  cmd.group = group;
  cmd.name = name;
  cmd.shortcut = shortcut;
  cmd.callback = [=] () {
    if (cancel_other_named_processes) {
      CancelAllNamedProcesses(*app);
    }
    ScheduleProcess<P>(*app, process_name, args...);
  };
  app->user_command_events_ordered.append(event_type);
}

AppData::AppData(int argc, char** argv)
    : gui_app(argc, argv),
      ui_action_router(*this) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  user_config_path = home + "/.cpp-dev-tools.json";
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
  io_thread_pool.setMaxThreadCount(1);
  InitializeUI(*this);
  RegisterUserCmd<SearchUserCmds>(this, "searchUserCmds", "General",
                                  "Execute Command", "Ctrl+P", kDialogSlot,
                                  false);
  RegisterUserCmd<CloseProject>(this, "closeProject", "General",
                                "Close Project", "Ctrl+W", kViewSlot, true);
  RegisterUserCmd<ExecTasks>(this, "execTasks", "Task", "Execute Tasks",
                             "Ctrl+1", kViewSlot, true);
}
