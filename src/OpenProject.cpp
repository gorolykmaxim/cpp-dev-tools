#include "OpenProject.hpp"
#include "UserConfig.hpp"
#include "Threads.hpp"
#include "UI.hpp"
#include "Process.hpp"

static bool IsValid(const FileSuggestion& s) {
  return s.match_start >= 0 && s.match_start < s.file.size();
}

static bool Compare(const FileSuggestion& a, const FileSuggestion& b) {
  bool is_a_dir = a.file.endsWith('/');
  bool is_b_dir = b.file.endsWith('/');
  if (a.match_start != b.match_start) {
    return a.match_start < b.match_start;
  } else if (a.distance != b.distance) {
    return a.distance < b.distance;
  } else if (is_a_dir != is_b_dir) {
    return is_a_dir > is_b_dir;
  } else {
    return a.file < b.file;
  }
}

static void UpdateAndDisplaySuggestions(OpenProject& data, AppData& app) {
  data.selected_suggestion = 0;
  QString path = data.file_name.toLower();
  for (FileSuggestion& suggestion: data.suggestions) {
    QString file = suggestion.file.toLower();
    suggestion.match_start = file.indexOf(path);
    if (suggestion.match_start < 0) {
      suggestion.match_start = std::numeric_limits<int>::max();
    }
    if (path.isEmpty()) {
      suggestion.distance = std::numeric_limits<int>::max();
    } else {
      suggestion.distance = file.size() - path.size();
    }
  }
  std::sort(data.suggestions.begin(), data.suggestions.end(), Compare);
  QList<QVariantList> items;
  for (const FileSuggestion& suggestion: data.suggestions) {
    if (path.isEmpty() || IsValid(suggestion)) {
      QString file = suggestion.file;
      if (IsValid(suggestion)) {
        file.insert(suggestion.match_start + path.size(), "</b>");
        file.insert(suggestion.match_start, "<b>");
      }
      items.append({file});
    }
  }
  GetUIListField(app, kViewSlot, "vSuggestions").SetItems(items);
  QString button_text = data.HasValidSuggestionAvailable() ? "Open" : "Create";
  SetUIDataField(app, kViewSlot, "vButtonText", button_text);
}

class ReloadFileList: public Process {
public:
  ReloadFileList(const QString& path, OpenProject& root)
      : path(path), root(root) {
    EXEC_NEXT(Query);
  }
private:
  void Query(AppData& app) {
    QPtr<ReloadFileList> self = ProcessSharedPtr(app, this);
    ScheduleIOTask(app, [self] () {
      self->files = QDir(self->path).entryInfoList();
      for (QFileInfo& file: self->files) {
        file.stat();
      }
    }, [self, &app] () {WakeUpAndExecuteProcess(app, *self);});
    EXEC_NEXT(UpdateList);
  }

  void UpdateList(AppData& app) {
    root.suggestions.clear();
    for (const QFileInfo& file: files) {
      FileSuggestion suggestion;
      suggestion.file = file.fileName();
      if (suggestion.file == "." || suggestion.file == "..") {
        continue;
      }
      if (file.isDir()) {
        suggestion.file += '/';
      }
      root.suggestions.append(suggestion);
    }
    UpdateAndDisplaySuggestions(root, app);
  }

  QString path;
  QFileInfoList files;
  OpenProject& root;
};

OpenProject::OpenProject() {
  EXEC_NEXT(DisplayOpenProjectView);
}

void OpenProject::DisplayOpenProjectView(AppData& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  DisplayView(
      app,
      kViewSlot,
      "OpenProjectView.qml",
      {
        UIDataField{kWindowTitle, "Open Project"},
        UIDataField{"vPath", home + '/'},
        UIDataField{"vButtonText", "Open"},
      },
      {
        UIListField{"vSuggestions", {{0, "title"}}, QList<QVariantList>()},
      },
      {"vaPathChanged", "vaSuggestionPicked"});
  WakeUpProcessOnEvent(app, "vaPathChanged", *this,
                       EXEC(this, ChangeProjectPath));
  WakeUpProcessOnEvent(app, "vaSuggestionPicked", *this,
                       EXEC(this, HandleItemSelected));
}

void OpenProject::ChangeProjectPath(AppData& app) {
  QString new_path = GetEventArg(app, 0).toString();
  QString old_folder = folder;
  qsizetype i = new_path.lastIndexOf('/');
  folder = i < 0 ? "/" : new_path.sliced(0, i + 1);
  file_name = i < 0 ? new_path : new_path.sliced(i + 1);
  if (old_folder != folder) {
    get_files = ReScheduleProcess<ReloadFileList>(app, get_files.get(), this,
                                                  folder, *this);
  }
  UpdateAndDisplaySuggestions(*this, app);
  EXEC_NEXT(KeepAlive);
}

void OpenProject::HandleItemSelected(AppData& app) {
  selected_suggestion = GetEventArg(app, 0).toInt();
  if (HasValidSuggestionAvailable()) {
    QString value = folder + suggestions[selected_suggestion].file;
    SetUIDataField(app, kViewSlot, "vPath", value);
    if (!value.endsWith('/')) {
      qDebug() << "Opening project:" << value;
      load_project_file = ReScheduleProcess<JsonFileProcess>(
          app, load_project_file.get(), this, JsonOperation::kRead, value);
      EXEC_AND_WAIT_FOR_NEXT(LoadProjectFile);
    } else {
      EXEC_NEXT(Noop);
    }
  } else {
    DisplayAlertDialog(
        app,
        "Create new project?",
        "Do you want to create a new project at " + folder + file_name,
        false,
        true);
    WakeUpProcessOnEvent(app, "daOk", *this, EXEC(this, CreateNewProject));
  }
}

void OpenProject::CreateNewProject(AppData& app) {
  QString value = folder + file_name;
  qDebug() << "Creating project:" << value;
  load_project_file = ReScheduleProcess<JsonFileProcess>(
      app, load_project_file.get(), this, JsonOperation::kWrite, value,
      QJsonDocument());
  EXEC_NEXT(LoadProjectFile);
}

bool OpenProject::HasValidSuggestionAvailable() const {
  if (selected_suggestion < 0 || selected_suggestion >= suggestions.size()) {
    return false;
  }
  return IsValid(suggestions[selected_suggestion]);
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

static void DisplayStatusBar(AppData& app) {
  if (app.projects.isEmpty()) {
    return;
  }
  const Project& project = app.projects[0];
  QList<QVariantList> itemsLeft, itemsRight;
  itemsLeft.append({project.GetPathRelativeToHome()});
  if (project.profile >= 0) {
    QString profile_name = app.profiles[project.profile].GetName();
    itemsRight.append({"Profile: " + profile_name});
  }
  DisplayStatusBar(app, itemsLeft, itemsRight);
}

void OpenProject::LoadProjectFile(AppData& app) {
  if (!load_project_file->error.isEmpty()) {
    DisplayAlertDialog(app, "Failed to open project", load_project_file->error);
    EXEC_NEXT(KeepAlive);
    return;
  }
  QStringList errors;
  QList<Profile> profiles;
  QList<Task> task_defs;
  LoadProfiles(load_project_file->json, profiles, errors);
  LoadTasks(load_project_file->json, task_defs, errors);
  QList<Task> tasks = task_defs;
  Project project(load_project_file->path);
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
    SaveToUserConfig(app);
    // TODO:
    // - display something more useful as a title
    // - once shortcuts are implemented - display actual configured shortcut
    DisplayStatusBar(app);
    DisplayView(
        app,
        kViewSlot,
        "BlankView.qml",
        {
          UIDataField{kWindowTitle, "CPP Dev Tools"},
          UIDataField{"vText", "Execute Command: <b>\u2318O</b>"},
        },
        {},
        {});
  }
  EXEC_NEXT(KeepAlive);
}
