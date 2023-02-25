#include "ChooseFileController.hpp"

#include <algorithm>
#include <limits>

#include "Application.hpp"
#include "QVariantListModel.hpp"

FileSuggestionListModel::FileSuggestionListModel() {
  SetRoleNames({{0, "idx"}, {1, "title"}});
  searchable_roles = {1};
}

QVariantList FileSuggestionListModel::GetRow(int i) const {
  return {i, list[i].file};
}

int FileSuggestionListModel::GetRowCount() const { return list.size(); }

ChooseFileController::ChooseFileController(QObject* parent)
    : QObject(parent), suggestions(new FileSuggestionListModel()) {
  SetPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + '/');
}

ChooseFileController::~ChooseFileController() { suggestions->deleteLater(); }

QString ChooseFileController::GetPath() const { return folder + file; }

void ChooseFileController::SetPath(const QString& path) {
  if (path == GetPath()) {
    return;
  }
  QString old_folder = folder;
  qsizetype i = path.lastIndexOf('/');
  folder = i < 0 ? "/" : path.sliced(0, i + 1);
  file = i < 0 ? path : path.sliced(i + 1);
  if (old_folder != folder) {
    QString path = folder;
    Application::Get().RunIOTask<QList<FileSuggestion>>(
        this,
        [path]() {
          QList<FileSuggestion> result;
          for (const QFileInfo& file : QDir(path).entryInfoList()) {
            FileSuggestion suggestion;
            suggestion.file = file.fileName();
            if (suggestion.file == "." || suggestion.file == ".." ||
                !file.isDir()) {
              continue;
            }
            suggestion.file += '/';
            result.append(suggestion);
          }
          return result;
        },
        [this](QList<FileSuggestion> result) {
          suggestions->list = result;
          SortAndFilterSuggestions();
        });
  } else {
    SortAndFilterSuggestions();
  }
  emit pathChanged();
}

void ChooseFileController::PickSuggestion(int i) {
  SetPath(folder + suggestions->list[i].file);
}

void ChooseFileController::OpenOrCreateFile() {
  QString path = GetPath();
  Application::Get().RunIOTask<bool>(
      this, [path]() { return QFile(path).exists(); },
      [this](bool exists) {
        if (exists) {
          emit fileChosen();
        } else {
          emit willCreateFile();
        }
      });
}

static bool Compare(const FileSuggestion& a, const FileSuggestion& b) {
  if (a.match_start != b.match_start) {
    return a.match_start < b.match_start;
  } else if (a.distance != b.distance) {
    return a.distance < b.distance;
  } else {
    return a.file < b.file;
  }
}

void ChooseFileController::SortAndFilterSuggestions() {
  for (FileSuggestion& suggestion : suggestions->list) {
    suggestion.match_start =
        suggestion.file.indexOf(file, Qt::CaseSensitivity::CaseInsensitive);
    if (suggestion.match_start < 0) {
      suggestion.match_start = std::numeric_limits<int>::max();
    }
    if (file.isEmpty()) {
      suggestion.distance = std::numeric_limits<int>::max();
    } else {
      suggestion.distance = suggestion.file.size() - file.size();
    }
  }
  std::sort(suggestions->list.begin(), suggestions->list.end(), Compare);
  suggestions->SetFilter(file);
}
