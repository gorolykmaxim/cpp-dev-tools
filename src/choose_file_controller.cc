#include "choose_file_controller.h"

#include "application.h"
#include "io_task.h"
#include "qvariant_list_model.h"
#include "view_system.h"

#define LOG() qDebug() << "[ChooseFileController]"

ChooseFileController::ChooseFileController(QObject* parent)
    : QObject(parent), suggestions(new FileSuggestionListModel(this)) {
  SetPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + '/');
  suggestions->min_filter_sub_match_length = 1;
}

QString ChooseFileController::GetPath() const { return folder + file; }

bool ChooseFileController::CanOpen() const {
  if (choose_folder) {
    return allow_creating || (file.isEmpty() && !folder.isEmpty());
  } else {
    int i = suggestions->IndexOfSuggestionWithName(file);
    if (i < 0) {
      return allow_creating && !file.isEmpty();
    } else {
      return !suggestions->list[i].is_dir;
    }
  }
}

static bool Compare(const FileSuggestion& a, const FileSuggestion& b) {
  if (a.is_dir != b.is_dir) {
    return a.is_dir > b.is_dir;
  } else {
    return a.name < b.name;
  }
}

void ChooseFileController::SetPath(const QString& path) {
  if (path == GetPath()) {
    return;
  }
  QString old_folder = folder;
  qsizetype i = path.lastIndexOf('/');
  folder = i < 0 ? "/" : path.sliced(0, i + 1);
  file = i < 0 ? path : path.sliced(i + 1);
  LOG() << "Chaning path to" << path << "folder:" << folder << "file:" << file;
  if (old_folder != folder) {
    LOG() << "Will refresh list of file suggestions";
    QString path = folder;
    bool choose_folder = this->choose_folder;
    IoTask::Run<QList<FileSuggestion>>(
        this,
        [path, choose_folder]() {
          QList<FileSuggestion> results;
          QDir::Filters filters = QDir::Dirs | QDir::NoDotAndDotDot;
          if (!choose_folder) {
            filters |= QDir::Files;
          }
          QDirIterator it(path, filters);
          while (it.hasNext()) {
            FileSuggestion suggestion;
            suggestion.name = it.next();
            suggestion.name.remove(path);
            suggestion.is_dir = it.fileInfo().isDir();
            results.append(suggestion);
          }
          std::sort(results.begin(), results.end(), Compare);
          return results;
        },
        [this](QList<FileSuggestion> results) {
          suggestions->list = results;
          if (!suggestions->SetFilter(file)) {
            suggestions->Load();
          }
        });
  } else {
    suggestions->SetFilter(file);
  }
  emit pathChanged();
}

void ChooseFileController::pickSuggestion(int i) {
  const FileSuggestion& suggestion = suggestions->list[i];
  QString path = folder + suggestion.name;
  if (suggestion.is_dir) {
    path += '/';
  }
  SetPath(path);
}

void ChooseFileController::openOrCreateFile() {
  if (!CanOpen()) {
    return;
  }
  QString path = GetResultPath();
  IoTask::Run<bool>(
      this,
      [path]() {
        LOG() << "Checking if" << path << "exists";
        return QFile::exists(path);
      },
      [this, path](bool exists) {
        Application& app = Application::Get();
        if (exists) {
          emit fileChosen(path);
        } else {
          QString type = choose_folder ? "folder" : "file";
          app.view.DisplayAlertDialog(
              "Create new " + type + '?',
              "Do you want to create a new " + type + ": " + path + "?");
          QObject::connect(&app.view, &ViewSystem::alertDialogAccepted, this,
                           &ChooseFileController::CreateFile);
        }
      });
}

void ChooseFileController::CreateFile() {
  QString path = GetResultPath();
  LOG() << "Creating" << path;
  bool choose_folder = this->choose_folder;
  QString folder = this->folder;
  IoTask::Run(
      this,
      [path, choose_folder, folder] {
        if (choose_folder) {
          QDir().mkpath(path);
        } else {
          QDir().mkpath(folder.sliced(0, folder.size() - 1));
          QFile(path).open(QFile::WriteOnly);
        }
      },
      [this, path] { emit fileChosen(path); });
}

QString ChooseFileController::GetResultPath() const {
  QString path = GetPath();
  if (path.endsWith('/')) {
    path = path.sliced(0, path.length() - 1);
  }
  return path;
}

FileSuggestionListModel::FileSuggestionListModel(QObject* parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "idx"}, {1, "title"}, {2, "icon"}});
  searchable_roles = {1};
}

int FileSuggestionListModel::IndexOfSuggestionWithName(
    const QString& name) const {
  for (int i = 0; i < list.size(); i++) {
    if (list[i].name == name) {
      return i;
    }
  }
  return -1;
}

QVariantList FileSuggestionListModel::GetRow(int i) const {
  const FileSuggestion& suggestion = list[i];
  QString icon;
  if (suggestion.is_dir) {
    icon = "folder_open";
  } else {
    icon = "insert_drive_file";
  }
  return {i, suggestion.name, icon};
}

int FileSuggestionListModel::GetRowCount() const { return list.size(); }
