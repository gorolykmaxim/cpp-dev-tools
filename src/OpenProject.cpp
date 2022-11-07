#include "OpenProject.hpp"

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

static void UpdateAndDisplaySuggestions(OpenProject& data, UserInterface& ui) {
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
  QVector<QVariantList> items;
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
  ui.SetInputAndListViewItems(items);
  QString button_text = data.HasValidSuggestionAvailable() ? "Open" : "Create";
  ui.SetInputAndListViewButtonText(button_text);
}

class ReloadFileList: public Process {
public:
  ReloadFileList(const QString& path, OpenProject& root)
      : path(path), root(root) {
    EXEC_NEXT(Query);
  }
private:
  void Query(Application& app) {
    QPtr<ReloadFileList> self = app.runtime.SharedPtr(this);
    app.threads.ScheduleIO([self] () {
      self->files = QDir(self->path).entryInfoList();
      for (QFileInfo& file: self->files) {
        file.stat();
      }
    }, [self, &app] () {app.runtime.WakeUpAndExecute(*self);});
    EXEC_NEXT(UpdateList);
  }

  void UpdateList(Application& app) {
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
    UpdateAndDisplaySuggestions(root, app.ui);
  }

  QString path;
  QFileInfoList files;
  OpenProject& root;
};

OpenProject::OpenProject() {
  EXEC_NEXT(DisplayOpenProjectView);
}

void OpenProject::DisplayOpenProjectView(Application& app) {
  QPtr<OpenProject> self = app.runtime.SharedPtr(this);
  app.ui.DisplayInputAndListView(
      "Open project by path:",
      QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + '/',
      "Open",
      QVector<QVariantList>(),
      [self, &app] (const QString& value) {
        self->ChangeProjectPath(value, app);
      },
      [self, &app] () {
        self->HandleEnter(app);
      },
      [self] (int item) {
        self->selected_suggestion = item;
      });
  EXEC_NEXT(KeepAlive);
}

void OpenProject::ChangeProjectPath(const QString& new_path, Application& app) {
  QString old_folder = folder;
  qsizetype i = new_path.lastIndexOf('/');
  folder = i < 0 ? "/" : new_path.sliced(0, i + 1);
  file_name = i < 0 ? new_path : new_path.sliced(i + 1);
  if (old_folder != folder) {
    get_files = app.runtime.ReScheduleAndExecute<ReloadFileList>(
        get_files.get(), this, folder, *this);
  }
  UpdateAndDisplaySuggestions(*this, app.ui);
}

void OpenProject::HandleEnter(Application& app) {
  if (HasValidSuggestionAvailable()) {
    QString value = folder + suggestions[selected_suggestion].file;
    app.ui.SetInputAndListViewInput(value);
    if (!value.endsWith('/')) {
      qDebug() << "Opening project:" << value;
      load_project_file = app.runtime.ReScheduleAndExecute<JsonFileProcess>(
          load_project_file.get(), this, JsonOperation::kRead, value);
      EXEC_NEXT(LoadProjectFile);
    }
  } else {
    QPtr<OpenProject> self = app.runtime.SharedPtr(this);
    QString value = folder + file_name;
    app.ui.DisplayAlertDialog(
        "Create new project?",
        "Do you want to create a new project at " + value,
        false,
        true,
        [self, &app] () {
          app.runtime.WakeUpAndExecute(*self, EXEC(self, CreateNewProject));
        });
  }
}

void OpenProject::CreateNewProject(Application& app) {
  QString value = folder + file_name;
  qDebug() << "Creating project:" << value;
  load_project_file = app.runtime.ReScheduleAndExecute<JsonFileProcess>(
      load_project_file.get(), this, JsonOperation::kWrite, value,
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

static void LoadProfiles(const QJsonDocument& json, QVector<Profile>& profiles,
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

static void LoadTasks(const QJsonDocument& json, QVector<Task>& tasks,
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

static void ApplyProfile(QVector<Task>& tasks, const Profile& profile) {
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

static void MigrateOldFormatTasks(QVector<Task>& tasks) {
  qDebug() << "Migrating tasks from the old format to the new one";
  for (Task& task: tasks) {
    MigrateTaskField(task, "__gtest", kTaskGtest);
    MigrateTaskField(task, "__restart", kTaskRestart);
  }
}

static void ValidateTasks(const QVector<Task>& tasks, QStringList& errors) {
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

void OpenProject::LoadProjectFile(Application& app) {
  if (!load_project_file->error.isEmpty()) {
    app.ui.DisplayAlertDialog("Failed to open project",
                              load_project_file->error);
    EXEC_NEXT(KeepAlive);
    return;
  }
  QStringList errors;
  QVector<Profile> profiles;
  QVector<Task> task_defs;
  LoadProfiles(load_project_file->json, profiles, errors);
  LoadTasks(load_project_file->json, task_defs, errors);
  QVector<Task> tasks = task_defs;
  if (!profiles.isEmpty()) {
    ApplyProfile(tasks, profiles[0]);
  }
  MigrateOldFormatTasks(tasks);
  ValidateTasks(tasks, errors);
  // TODO: remove once tasks become viewable in UI
  for (const Task& task: tasks) {
    qDebug() << task.name << task.command << task.flags << task.pre_tasks;
  }
  if (!errors.isEmpty()) {
    app.ui.DisplayAlertDialog("Failed to open project", errors.join('\n'));
  } else {
    app.profiles = profiles;
    app.task_defs = task_defs;
    app.tasks = tasks;
  }
  EXEC_NEXT(KeepAlive);
}
