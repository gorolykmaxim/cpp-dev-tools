#include "OpenProject.hpp"
#include <QDir>
#include <QVariant>
#include <QStandardPaths>
#include <algorithm>
#include <limits>
#include "Dialog.hpp"
#include "InputAndListView.hpp"

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
  InputAndListView::SetItems(items, ui);
  QString button_text = data.HasValidSuggestionAvailable() ? "Open" : "Create";
  InputAndListView::SetButtonText(button_text, ui);
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
  InputAndListView::Display(
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
      },
      app.ui);
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
    InputAndListView::SetInput(value, app.ui);
    if (!value.endsWith('/')) {
      qDebug() << "Opening project:" << value;
      load_project_file = app.runtime.ReScheduleAndExecute<JsonFileProcess>(
          load_project_file.get(), this, JsonOperation::kRead, value);
      EXEC_NEXT(LoadProjectFile);
    }
  } else {
    QString value = folder + file_name;
    qDebug() << "Creating project:" << value;
    load_project_file = app.runtime.ReScheduleAndExecute<JsonFileProcess>(
        load_project_file.get(), this, JsonOperation::kWrite, value,
        QJsonDocument());
    EXEC_NEXT(LoadProjectFile);
  }
}

bool OpenProject::HasValidSuggestionAvailable() const {
  if (selected_suggestion < 0 || selected_suggestion >= suggestions.size()) {
    return false;
  }
  return IsValid(suggestions[selected_suggestion]);
}

void OpenProject::LoadProjectFile(Application& app) {
  if (!load_project_file->error.isEmpty()) {
    Dialog::DisplayError("Failed to open project", load_project_file->error,
                         app.ui);
  } else {
    qDebug() << load_project_file->json;
  }
  EXEC_NEXT(KeepAlive);
}
