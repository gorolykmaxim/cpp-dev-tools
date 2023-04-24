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
  Application &app = Application::Get();
  QList<Database::Cmd> cmds;
  if (!settings.open_in_editor_command.contains("{}")) {
    settings.open_in_editor_command += " {}";
  }
  if (settings.open_in_editor_command.trimmed() != "{}") {
    app.editor.open_command = settings.open_in_editor_command;
    cmds.append(Database::Cmd("UPDATE editor SET open_command=?",
                              {settings.open_in_editor_command}));
  }
  cmds.append(Database::Cmd("DELETE FROM external_search_folder"));
  for (const QString &folder : external_search_folders->list) {
    cmds.append(Database::Cmd("INSERT INTO external_search_folder VALUES(?)",
                              {folder}));
  }
  app.task.history_limit = settings.task_history_limit;
  cmds.append(Database::Cmd("UPDATE task_context SET history_limit=?",
                            {settings.task_history_limit}));
  Database::ExecCmdsAsync(cmds);
  emit settingsChanged();
}

void SettingsController::Load() {
  LOG() << "Loading settings";
  IoTask::Run<Settings>(
      this,
      [] {
        Settings settings;
        settings.external_search_folders = Database::ExecQueryAndRead<QString>(
            "SELECT * FROM external_search_folder",
            &Database::ReadStringFromSql);
        settings.task_history_limit =
            Database::ExecQueryAndRead<int>(
                "SELECT history_limit FROM task_context",
                &Database::ReadIntFromSql)
                .first();
        return settings;
      },
      [this](Settings settings) {
        external_search_folders->list = settings.external_search_folders;
        external_search_folders->Load();
        settings.open_in_editor_command =
            Application::Get().editor.open_command;
        this->settings = settings;
        emit settingsChanged();
      });
}

FolderListModel::FolderListModel(QObject *parent) : QVariantListModel(parent) {
  SetRoleNames({{0, "title"}});
  searchable_roles = {0};
}

QVariantList FolderListModel::GetRow(int i) const { return {list[i]}; }

int FolderListModel::GetRowCount() const { return list.size(); }

bool Settings::operator==(const Settings &another) const {
  return open_in_editor_command == another.open_in_editor_command &&
         task_history_limit == another.task_history_limit &&
         external_search_folders == another.external_search_folders;
}

bool Settings::operator!=(const Settings &another) const {
  return !(*this == another);
}
