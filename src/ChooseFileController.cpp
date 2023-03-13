#include "ChooseFileController.hpp"

#include "Application.hpp"
#include "QVariantListModel.hpp"
#include "ViewSystem.hpp"

#define LOG() qDebug() << "[ChooseFileController]"

ChooseFileController::ChooseFileController(QObject* parent)
    : QObject(parent),
      suggestions(
          new SimpleQVariantListModel(this, {{0, "idx"}, {1, "title"}}, {1})) {
  SetPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + '/');
}

QString ChooseFileController::GetPath() const { return folder + file; }

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
    Application::Get().RunIOTask<QStringList>(
        this,
        [path]() {
          QStringList results;
          for (const QFileInfo& file : QDir(path).entryInfoList()) {
            if (file.fileName() == "." || file.fileName() == ".." ||
                !file.isDir()) {
              continue;
            }
            results.append(file.fileName() + '/');
          }
          results.sort();
          return results;
        },
        [this](QStringList results) {
          suggestions->list.clear();
          for (int i = 0; i < results.size(); i++) {
            suggestions->list.append({i, results[i]});
          }
          suggestions->SetFilter(file);
        });
  } else {
    suggestions->SetFilter(file);
  }
  emit pathChanged();
}

void ChooseFileController::PickSuggestion(int i) {
  SetPath(folder + suggestions->list[i][1].toString());
}

void ChooseFileController::OpenOrCreateFile() {
  Application& app = Application::Get();
  QString path = GetResultPath();
  app.RunIOTask<bool>(
      this,
      [path]() {
        LOG() << "Checking if" << path << "exists";
        return QFile(path).exists();
      },
      [this, &app, path](bool exists) {
        if (exists) {
          emit fileChosen(path);
        } else {
          app.view.DisplayAlertDialog(
              "Create new folder?",
              "Do you want to create a new folder: " + path + "?");
          QObject::connect(&app.view, &ViewSystem::alertDialogAccepted, this,
                           &ChooseFileController::CreateFile);
        }
      });
}

void ChooseFileController::CreateFile() {
  QString path = GetResultPath();
  LOG() << "Creating" << path;
  Application::Get().RunIOTask(
      this, [path] { QDir().mkpath(path); },
      [this, path] { emit fileChosen(path); });
}

QString ChooseFileController::GetResultPath() const {
  QString path = GetPath();
  if (path.endsWith('/')) {
    path = path.sliced(0, path.length() - 1);
  }
  return path;
}
