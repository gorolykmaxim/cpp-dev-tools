#include "settings_controller.h"

#include "application.h"
#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[SettingsController]"

SettingsController::SettingsController(QObject *parent)
    : QObject(parent), external_search_folders(new FolderListModel(this)) {
  Application::Get().view.SetWindowTitle("Settings");
  Load();
}

void SettingsController::configureExternalSearchFolders() {
  LOG() << "Opening external search folder editor";
  Application::Get().view.SetWindowTitle("External Search Folders");
  emit openExternalSearchFoldersEditor();
}

void SettingsController::addExternalSearchFolder(const QString &folder) {
  if (external_search_folders->list.contains(folder)) {
    return;
  }
  LOG() << "Adding external search folder" << folder;
  external_search_folders->list.append(folder);
  external_search_folders->Load();
}

void SettingsController::removeExternalSearchFolder(const QString &folder) {
  LOG() << "Removing external search folder" << folder;
  external_search_folders->list.removeAll(folder);
  external_search_folders->Load();
}

void SettingsController::goToSettings() {
  LOG() << "Opening settings";
  Application::Get().view.SetWindowTitle("Settings");
  emit openSettings();
}

void SettingsController::save() {
  LOG() << "Saving settings";
  QList<Database::Cmd> cmds;
  if (!open_in_editor_command.contains("{}")) {
    open_in_editor_command += " {}";
  }
  if (open_in_editor_command.trimmed() != "{}") {
    Application::Get().editor.open_command = open_in_editor_command;
    cmds.append(Database::Cmd("UPDATE editor SET open_command=?",
                              {open_in_editor_command}));
  }
  cmds.append(Database::Cmd("DELETE FROM external_search_folder"));
  for (const QString &folder : external_search_folders->list) {
    cmds.append(Database::Cmd("INSERT INTO external_search_folder VALUES(?)",
                              {folder}));
  }
  Database::ExecCmdsAsync(cmds);
  emit settingsChanged();
}

void SettingsController::Load() {
  LOG() << "Loading settings";
  IoTask::Run<QList<QString>>(
      this,
      [] {
        return Database::ExecQueryAndRead<QString>(
            "SELECT * FROM external_search_folder",
            &Database::ReadStringFromSql);
      },
      [this](QList<QString> folders) {
        external_search_folders->list = folders;
        external_search_folders->Load();
        open_in_editor_command = Application::Get().editor.open_command;
        emit settingsChanged();
      });
}

FolderListModel::FolderListModel(QObject *parent) : QVariantListModel(parent) {
  SetRoleNames({{0, "title"}});
  searchable_roles = {0};
}

QVariantList FolderListModel::GetRow(int i) const { return {list[i]}; }

int FolderListModel::GetRowCount() const { return list.size(); }
