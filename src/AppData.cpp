#include "AppData.hpp"
#include "UI.hpp"
#include "Process.hpp"
#include "SearchUserCommands.hpp"

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

QString UserCommand::GetFormattedShortcut() const {
  QString result = shortcut.toUpper();
#if __APPLE__
  result.replace("CTRL", "\u2318");
#endif
  return result;
}

template<typename P, typename... Args>
static void RegisterUserCommand(AppData* app, const QString& event_type,
                                const QString& group, const QString& name,
                                const QString& shortcut, Args&&... args) {
  UserCommand& cmd = app->user_commands[event_type];
  cmd.group = group;
  cmd.name = name;
  cmd.shortcut = shortcut;
  cmd.callback = [=] () {
    ScheduleProcess<P>(*app, nullptr, args...);
  };
}

AppData::AppData(int argc, char** argv)
    : gui_app(argc, argv),
      ui_action_router(*this) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  user_config_path = home + "/.cpp-dev-tools.json";
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
  io_thread_pool.setMaxThreadCount(1);
  InitializeUI(*this);
  RegisterUserCommand<SearchUserCommands>(this, "searchUserCommands", "General",
                                          "Execute Command", "Ctrl+P");
}
