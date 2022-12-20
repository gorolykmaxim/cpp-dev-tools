#include "OpenProject.hpp"
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
        UIDataField{"windowTitle", "Open Project"},
        UIDataField{"vPath", home + '/'},
        UIDataField{"vButtonText", "Open"},
      },
      {
        UIListField{"vSuggestions", {{0, "title"}}, QList<QVariantList>()},
      });
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaPathChanged", *this,
                         EXEC(this, ChangeProjectPath));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaSuggestionPicked", *this,
                         EXEC(this, HandleItemSelected));
  // In case of cancelling - just finish this process
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaOpeningCancelled", *this,
                         EXEC(this, Noop));
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
      load_project_file = ScheduleProcess<LoadTaskConfig>(app, this, value);
      EXEC_AND_WAIT_FOR_NEXT(HandleOpeningCompletion);
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
    WakeUpProcessOnUIEvent(app, kDialogSlot, "daOk", *this,
                           EXEC(this, CreateNewProject));
  }
}

void OpenProject::CreateNewProject(AppData& app) {
  QString value = folder + file_name;
  qDebug() << "Creating project:" << value;
  load_project_file = ScheduleProcess<LoadTaskConfig>(app, this, value, true);
  EXEC_AND_WAIT_FOR_NEXT(HandleOpeningCompletion);
}

void OpenProject::HandleOpeningCompletion(AppData&) {
  opened = load_project_file->success;
  if (!opened) {
    EXEC_NEXT(KeepAlive);
  }
}

bool OpenProject::HasValidSuggestionAvailable() const {
  if (selected_suggestion < 0 || selected_suggestion >= suggestions.size()) {
    return false;
  }
  return IsValid(suggestions[selected_suggestion]);
}
