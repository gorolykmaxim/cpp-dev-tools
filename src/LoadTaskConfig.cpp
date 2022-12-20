#include "LoadTaskConfig.hpp"
#include "Process.hpp"
#include "UI.hpp"
#include "SaveUserConfig.hpp"

LoadTaskConfig::LoadTaskConfig(const QString& path, bool create)
    : create(create), path(path) {
  EXEC_NEXT(Load);
}

void LoadTaskConfig::Load(AppData& app) {
  if (create) {
    load_file = ScheduleProcess<JsonFileProcess>(app, this,
                                                 JsonOperation::kWrite,
                                                 path, QJsonDocument());
  } else {
    load_file = ScheduleProcess<JsonFileProcess>(app, this,
                                                 JsonOperation::kRead, path);
  }
  EXEC_NEXT(Read);
}

static void AppendError(QStringList& errors, const QString& type,
                        const QString& name, const QString& error) {
  QString full_error;
  QTextStream(&full_error) << type << " '" << name << "' " << error;
  errors << full_error;
}

static void AppendError(QStringList& errors, const QString& type, int index,
                        const QString& error) {
  AppendError(errors, type, QString::number(index + 1), error);
}

static void LoadProfiles(const QJsonDocument& json, QList<Profile>& profiles,
                         QStringList& errors) {
  qDebug() << "Loading profiles";
  QSet<QString> profile_variable_names;
  QSet<QString> profile_names;
  QJsonArray json_profiles = json["cdt_profiles"].toArray();
  for (int i = 0; i < json_profiles.size(); i++) {
    QJsonValue json_profile = json_profiles[i];
    if (!json_profile.isObject()) {
      AppendError(errors, "Profile", i, "must be an object");
      continue;
    }
    QJsonObject json_profile_obj = json_profile.toObject();
    Profile profile;
    for (const QString& key: json_profile_obj.keys()) {
      profile[key] = json_profile_obj[key].toString();
      profile_variable_names.insert(key);
    }
    QString name = profile.GetName();
    if (name.isEmpty()) {
      AppendError(errors, "Profile", i, "must have a 'name' variable set");
      continue;
    }
    if (profile_names.contains(name)) {
      AppendError(errors, "Profile", i, "has a name '" + name +
                  "' that conflicts with another profile");
      continue;
    }
    profile_names.insert(name);
    profiles.append(profile);
  }
  for (const Profile& profile: profiles) {
    for (const QString& variable: profile_variable_names) {
      if (!profile.Contains(variable)) {
        QString error = "is missing variable '" + variable +
                        "' defined in other profiles";
        AppendError(errors, "Profile", profile.GetName(), error);
      }
    }
  }
}

static void LoadTasks(const QJsonDocument& json, QList<Task>& tasks,
                      QStringList& errors) {
  qDebug() << "Loading tasks";
  QJsonArray json_tasks = json["cdt_tasks"].toArray();
  for (int i = 0; i < json_tasks.size(); i++) {
    QJsonValue json_task = json_tasks[i];
    if (!json_task.isObject()) {
      AppendError(errors, "Task", i, "must be an object");
      continue;
    }
    QJsonObject json_task_obj = json_task.toObject();
    Task task;
    bool is_valid = true;
    task.name = json_task_obj["name"].toString();
    if (task.name.isEmpty()) {
      AppendError(errors, "Task", i, "must have a 'name' property set");
      is_valid = false;
    }
    task.command = json_task_obj["command"].toString();
    if (task.command.isEmpty()) {
      AppendError(errors, "Task", i, "must have a 'command' property set");
      is_valid = false;
    }
    task.flags |= json_task_obj["is_restart"].toBool() ? kTaskRestart : 0;
    task.flags |= json_task_obj["is_gtest"].toBool() ? kTaskGtest : 0;
    QJsonValue json_pre_tasks = json_task_obj["pre_tasks"];
    static const QString kPreTasksErrMsg = "must have a 'pre_tasks' "
                                           "property set to an array of "
                                           "other task names";
    if (!json_pre_tasks.isNull() && !json_pre_tasks.isUndefined() &&
        !json_pre_tasks.isArray()) {
      AppendError(errors, "Task", i, kPreTasksErrMsg);
      is_valid = false;
    }
    QJsonArray json_pre_tasks_arr = json_pre_tasks.toArray();
    for (QJsonValue json_pre_task: json_pre_tasks_arr) {
      if (!json_pre_task.isString()) {
        AppendError(errors, "Task", i, kPreTasksErrMsg);
        is_valid = false;
        break;
      }
      task.pre_tasks.append(json_pre_task.toString());
    }
    if (is_valid) {
      tasks.append(task);
    }
  }
}

static void ApplyProfile(QString& str, const Profile& profile) {
  for (const QString& name: profile.GetVariableNames()) {
    str.replace("{" + name + "}", profile[name]);
  }
}

static void ApplyProfile(QList<Task>& tasks, const Profile& profile) {
  qDebug() << "Applying profile varaibles to tasks";
  for (Task& task: tasks) {
    ApplyProfile(task.name, profile);
    ApplyProfile(task.command, profile);
    for (QString& pre_task: task.pre_tasks) {
      ApplyProfile(pre_task, profile);
    }
  }
}

static void MigrateTaskField(Task& task, const QString& prefix, int task_flag) {
  if (task.command.startsWith(prefix)) {
    task.flags |= task_flag;
    task.command.remove(0, prefix.size());
    task.command = task.command.trimmed();
  }
}

static void MigrateOldFormatTasks(QList<Task>& tasks) {
  qDebug() << "Migrating tasks from the old format to the new one";
  for (Task& task: tasks) {
    MigrateTaskField(task, "__gtest", kTaskGtest);
    MigrateTaskField(task, "__restart", kTaskRestart);
  }
}

static void ValidateUniqueTaskNames(const QList<Task>& tasks,
                                    QStringList& errors) {
  QSet<QString> task_names;
  for (int i = 0; i < tasks.size(); i++) {
    const Task& task = tasks[i];
    if (task_names.contains(task.name)) {
      AppendError(errors, "Task", i, "has a name '" + task.name +
                  "' that conflicts with another task");
    } else {
      task_names.insert(task.name);
    }
  }
}

static void ExpandPreTasks(QList<Task> &tasks, QStringList& errors) {
  qDebug() << "Traversing pre task tree";
  QList<QList<int>> task_to_pre_tasks(tasks.size());
  for (int i = 0; i < tasks.size(); i++) {
    const Task& task = tasks[i];
    for (const QString& name: task.pre_tasks) {
      auto it = std::find_if(tasks.cbegin(),
                             tasks.cend(),
                             [&name] (const Task& t) {return t.name == name;});
      if (it == tasks.end()) {
        AppendError(errors, "Task", task.name, "in it's 'pre_tasks' references "
                    "a task '" + name + "' that does not exist");
      } else {
        task_to_pre_tasks[i].append(it - tasks.begin());
      }
    }
  }
  for (int i = 0; i < tasks.size(); i++) {
    Task& task = tasks[i];
    task.pre_tasks.clear();
    QStack<int> call_stack;
    QStack<std::pair<int, int>> to_visit;
    // to_visit pairs consist of:
    // 0 - index of pre task
    // 1 - index of the parent task of pre task
    call_stack.push(i);
    for (int j: task_to_pre_tasks[i]) {
      to_visit.push(std::make_pair(j, i));
    }
    while (!to_visit.isEmpty()) {
      auto [pre_task_idx, parent_idx] = to_visit.pop();
      // In a call tree A -> B, A -> C, C -> D, C -> E we might've just been in
      // E and now we've abruptly switched to B (because we've visited all the
      // nodes of the C, D, E sub-tree). The call stack is still A -> C right
      // now, so first we need to get rid of non-popped frames (C) before
      // adding B to the stack.
      while (call_stack.top() != parent_idx) {
        call_stack.pop();
      }
      call_stack.push(pre_task_idx);
      if (call_stack.count(pre_task_idx) > 1) {
        QStringList call_stack_str;
        for (int c: call_stack) {
          call_stack_str.append(tasks[c].name);
        }
        AppendError(errors, "Task", task.name, "has a circular dependency "
                    "in it's 'pre_tasks':\n" + call_stack_str.join(" -> "));
        break;
      }
      task.pre_tasks.insert(0, tasks[pre_task_idx].name);
      for (int pre_task_pre_task: task_to_pre_tasks[pre_task_idx]) {
        to_visit.push(std::make_pair(pre_task_pre_task, pre_task_idx));
      }
    }
  }
}

void LoadTaskConfig::Read(AppData& app) {
  if (!load_file->error.isEmpty()) {
    DisplayAlertDialog(app, "Failed to open project", load_file->error);
    return;
  }
  QStringList errors;
  QList<Profile> profiles;
  QList<Task> task_defs;
  LoadProfiles(load_file->json, profiles, errors);
  LoadTasks(load_file->json, task_defs, errors);
  QList<Task> tasks = task_defs;
  Project project(load_file->path);
  if (!profiles.isEmpty()) {
    project.profile = 0;
    ApplyProfile(tasks, profiles[project.profile]);
  }
  MigrateOldFormatTasks(tasks);
  ValidateUniqueTaskNames(tasks, errors);
  ExpandPreTasks(tasks, errors);
  if (!errors.isEmpty()) {
    DisplayAlertDialog(app, "Failed to open project", errors.join('\n'));
  } else {
    app.profiles = profiles;
    app.task_defs = task_defs;
    app.tasks = tasks;
    app.projects.removeOne(project);
    app.projects.insert(0, project);
    app.current_project_path = project.path;
    success = true;
    ScheduleProcess<SaveUserConfig>(app, this);
    DisplayMenuBar(app);
    DisplayStatusBar(app);
    UserCommand& main_cmd = app.user_commands["searchUserCommands"];
    QString text = main_cmd.name + ": <b>" + main_cmd.GetFormattedShortcut() +
                   "</b>";
    DisplayView(
        app,
        kViewSlot,
        "BlankView.qml",
        {
          UIDataField{"windowTitle", "CPP Dev Tools"},
          UIDataField{"vText", text},
        },
        {});
  }
}
