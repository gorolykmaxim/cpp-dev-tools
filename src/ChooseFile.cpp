#include "ChooseFile.hpp"

#include "Process.hpp"
#include "Threads.hpp"
#include "UI.hpp"

#define LOG() qDebug() << "[ChooseFile]"

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

static void UpdateAndDisplaySuggestions(ChooseFile& data, AppData& app) {
  data.selected_suggestion = 0;
  QString path = data.file_name.toLower();
  for (FileSuggestion& suggestion : data.suggestions) {
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
  for (int i = 0; i < data.suggestions.size(); i++) {
    FileSuggestion& suggestion = data.suggestions[i];
    if (path.isEmpty() || IsValid(suggestion)) {
      QString file = suggestion.file;
      if (IsValid(suggestion)) {
        file.insert(suggestion.match_start + path.size(), "</b>");
        file.insert(suggestion.match_start, "<b>");
      }
      items.append({file, i});
    }
  }
  GetUIListField(app, kViewSlot, "vSuggestions").SetItems(items);
}

class FetchFileInfo : public Process {
 public:
  FetchFileInfo(const QString& path, ChooseFile& root)
      : path(path), root(root) {
    EXEC_NEXT(Query);
  }

 private:
  void Query(AppData& app) {
    QPtr<FetchFileInfo> self = ProcessSharedPtr(app, this);
    ScheduleIOTask(
        app,
        [self]() {
          self->files = QDir(self->path).entryInfoList();
          for (QFileInfo& file : self->files) {
            file.stat();
          }
        },
        [self, &app]() { WakeUpAndExecuteProcess(app, *self); });
    EXEC_NEXT(UpdateList);
  }

  void UpdateList(AppData& app) {
    root.suggestions.clear();
    for (const QFileInfo& file : files) {
      FileSuggestion suggestion;
      suggestion.file = file.fileName();
      if (suggestion.file == "." || suggestion.file == ".." ||
          (!file.isDir() && root.choose_directory)) {
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
  ChooseFile& root;
};

ChooseFile::ChooseFile() : window_title("Open File"), choose_directory(false) {
  EXEC_NEXT(DisplayChooseFileView);
}

void ChooseFile::DisplayChooseFileView(AppData& app) {
  QHash<int, QByteArray> role_names = {{0, "title"}, {1, "idx"}};
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  DisplayView(
      app, kViewSlot, "ChooseFileView.qml",
      {
          UIDataField{"windowTitle", window_title},
          UIDataField{"vPath", home + '/'},
      },
      {
          UIListField{"vSuggestions", role_names, QList<QVariantList>()},
      });
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaPathChanged", *this,
                         EXEC(this, ChangePath));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaSuggestionPicked", *this,
                         EXEC(this, HandleItemSelected));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaOpenOrCreate", *this,
                         EXEC(this, HandleOpenOrCreate));
  // In case of cancelling - just finish this process
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaOpeningCancelled", *this,
                         EXEC(this, Noop));
}

void ChooseFile::ChangePath(AppData& app) {
  QString new_path = GetEventArg(app, 0).toString();
  QString old_folder = folder;
  qsizetype i = new_path.lastIndexOf('/');
  folder = i < 0 ? "/" : new_path.sliced(0, i + 1);
  file_name = i < 0 ? new_path : new_path.sliced(i + 1);
  if (old_folder != folder) {
    get_files = ReScheduleProcess<FetchFileInfo>(app, get_files.get(), this,
                                                 folder, *this);
  }
  UpdateAndDisplaySuggestions(*this, app);
  EXEC_NEXT(KeepAlive);
}

void ChooseFile::HandleItemSelected(AppData& app) {
  selected_suggestion = GetEventArg(app, 0).toInt();
  if (selected_suggestion >= 0 && selected_suggestion < suggestions.size() &&
      IsValid(suggestions[selected_suggestion])) {
    QString value = folder + suggestions[selected_suggestion].file;
    SetUIDataField(app, kViewSlot, "vPath", value);
  }
  EXEC_NEXT(KeepAlive);
}

void ChooseFile::HandleOpenOrCreate(AppData& app) {
  QString type = choose_directory ? "folder" : "file";
  QString value = MakeResult();
  QPtr<ChooseFile> self = ProcessSharedPtr(app, this);
  ScheduleIOTask<bool>(
      app, [value]() { return QFile(value).exists(); },
      [&app, self, value](bool file_exists) {
        if (file_exists) {
          self->result = value;
          WakeUpAndExecuteProcess(app, *self, EXEC(self, Noop));
        } else {
          WakeUpAndExecuteProcess(app, *self,
                                  EXEC(self, AskUserWhetherToCreateTheFile));
        }
      });
  EXEC_AND_WAIT_FOR_NEXT(Noop);
}

void ChooseFile::AskUserWhetherToCreateTheFile(AppData& app) {
  QString type = choose_directory ? "folder" : "file";
  QString value = MakeResult();
  DisplayAlertDialog(app, "Create new " + type + '?',
                     "Do you want to create a new " + type + ": " + value,
                     false, true);
  WakeUpProcessOnUIEvent(app, kDialogSlot, "daOk", *this,
                         EXEC(this, CreateNew));
}

void ChooseFile::CreateNew(AppData& app) {
  QString type = choose_directory ? "folder" : "file";
  QString value = MakeResult();
  result = value;
  LOG() << "Creating" << type + ':' << result;
  bool create_file = !choose_directory;
  QPtr<ChooseFile> self = ProcessSharedPtr(app, this);
  ScheduleIOTask(
      app,
      [value, create_file]() {
        if (create_file) {
          QString parent_folder = QDir::cleanPath(value + "/..");
          QDir().mkpath(parent_folder);
          QFile file(value);
          file.open(QIODevice::WriteOnly | QIODevice::Text);
        } else {
          QDir().mkpath(value);
        }
      },
      [&app, self]() { WakeUpAndExecuteProcess(app, *self); });
  EXEC_AND_WAIT_FOR_NEXT(Noop);
}

QString ChooseFile::MakeResult() const {
  QString value = folder + file_name;
  if (value.endsWith('/')) {
    value = value.sliced(0, value.length() - 1);
  }
  return value;
}
