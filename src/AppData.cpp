#include "AppData.hpp"

#include "CloseProject.hpp"
#include "ExecTasks.hpp"
#include "Process.hpp"
#include "SearchUserCmds.hpp"
#include "UI.hpp"

QString Project::GetPathRelativeToHome() const {
  QString home_str =
      QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  if (path.startsWith(home_str)) {
    return "~/" + QDir(home_str).relativeFilePath(path);
  } else {
    return path;
  }
}

QString Project::GetFolderName() const {
  qsizetype i = path.lastIndexOf('/');
  return i < 0 ? path : path.sliced(i + 1);
}

QDebug operator<<(QDebug debug, const Exec& exec) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "Exec(id=" << exec.id << ",peid=" << exec.primary_exec_id
                  << ",n=" << exec.task_name << ",c=" << exec.cmd
                  << ",st=" << exec.start_time << ",ec=";
  if (exec.exit_code) {
    debug << *exec.exit_code;
  } else {
    debug << "nullptr";
  }
  return debug << ",o=" << exec.output.size() << ')';
}

QString UserCmd::GetFormattedShortcut() const {
  QString result = shortcut.toUpper();
#if __APPLE__
  result.replace("CTRL", "\u2318");
#endif
  return result;
}

template <typename P, typename... Args>
static void RegisterUserCmd(AppData* app, const QString& event_type,
                            const QString& group, const QString& name,
                            const QString& shortcut,
                            const QString& process_name,
                            bool cancel_other_named_processes, Args&&... args) {
  UserCmd& cmd = app->user_cmds[event_type];
  cmd.group = group;
  cmd.name = name;
  cmd.shortcut = shortcut;
  cmd.callback = [=]() {
    if (cancel_other_named_processes) {
      CancelAllNamedProcesses(*app);
    }
    ScheduleProcess<P>(*app, process_name, args...);
  };
  app->user_command_events_ordered.append(event_type);
}

AppData::AppData(int argc, char** argv)
    : gui_app(argc, argv), ui_action_router(*this) {
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
