#include "settings_controller.h"

#include "application.h"
#include "io_task.h"

#define LOG() qDebug() << "[SettingsController]"

SettingsController::SettingsController(QObject *parent)
    : QObject(parent),
      external_search_folders(new FolderListModel(
          this, "external_search_folder", UiIcon("find_in_page"))),
      documentation_folders(new FolderListModel(this, "documentation_folder",
                                                UiIcon("description"))),
      terminals(new TerminalListModel(this)) {
  Application::Get().view.SetWindowTitle("Settings");
  Load();
}

void SettingsController::configureExternalSearchFolders() {
  LOG() << "Opening external search folder editor";
  Application::Get().view.SetWindowTitle("External Search Folders");
  emit openExternalSearchFoldersEditor();
}

void SettingsController::configureDocumentationFolders() {
  LOG() << "Opening documentation folder editor";
  Application::Get().view.SetWindowTitle("Documentation Folders");
  emit openDocumentationFoldersEditor();
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
  cmds.append(external_search_folders->MakeCommandsToUpdateDatabase());
  cmds.append(documentation_folders->MakeCommandsToUpdateDatabase());
  app.task.history_limit = settings.task_history_limit;
  cmds.append(Database::Cmd("UPDATE task_context SET history_limit=?",
                            {settings.task_history_limit}));
  for (int i = 0; i < terminals->list.size(); i++) {
    cmds.append(Database::Cmd("UPDATE terminal SET priority=? WHERE name=?",
                              {i, terminals->list[i]}));
  }
  Database::ExecCmdsAsync(cmds);
  emit settingsChanged();
  app.notification.Post(Notification("Settings: Changes saved"));
}

void SettingsController::moveSelectedTerminal(bool up) {
  int i = terminals->GetSelectedItemIndex();
  int new_i = up ? i - 1 : i + 1;
  if (i < 0 || new_i < 0 || new_i >= terminals->list.size()) {
    return;
  }
  terminals->list.swapItemsAt(i, new_i);
  terminals->Load(new_i);
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
        settings.documentation_folders = Database::ExecQueryAndRead<QString>(
            "SELECT * FROM documentation_folder", &Database::ReadStringFromSql);
        settings.task_history_limit =
            Database::ExecQueryAndRead<int>(
                "SELECT history_limit FROM task_context",
                &Database::ReadIntFromSql)
                .constFirst();
        settings.terminals = Database::ExecQueryAndRead<QString>(
            "SELECT name FROM terminal ORDER BY priority",
            &Database::ReadStringFromSql);
        return settings;
      },
      [this](Settings settings) {
        external_search_folders->SetFolders(settings.external_search_folders);
        documentation_folders->SetFolders(settings.documentation_folders);
        terminals->list = settings.terminals;
        terminals->Load(-1);
        settings.open_in_editor_command =
            Application::Get().editor.open_command;
        this->settings = settings;
        emit settingsChanged();
      });
}

FolderListModel::FolderListModel(QObject *parent, const QString &table,
                                 const UiIcon &icon)
    : TextListModel(parent), table(table), icon(icon) {
  SetRoleNames({{0, "title"}, {1, "icon"}});
  searchable_roles = {0};
  SetEmptyListPlaceholder("Add folders by clicking on 'Add Folder' button");
}

void FolderListModel::SetFolders(QStringList &folders) {
  list = folders;
  Load();
}

QList<Database::Cmd> FolderListModel::MakeCommandsToUpdateDatabase() {
  QList<Database::Cmd> cmds;
  cmds.append(Database::Cmd("DELETE FROM " + table));
  for (const QString &folder : list) {
    cmds.append(Database::Cmd("INSERT INTO " + table + " VALUES(?)", {folder}));
  }
  return cmds;
}

void FolderListModel::addFolder(const QString &folder) {
  if (list.contains(folder)) {
    return;
  }
  LOG() << "Adding" << table << folder;
  list.append(folder);
  Load();
}

void FolderListModel::removeFolder(const QString &folder) {
  LOG() << "Removing" << table << folder;
  list.removeAll(folder);
  Load();
}

QVariantList FolderListModel::GetRow(int i) const {
  return {list[i], icon.icon};
}

int FolderListModel::GetRowCount() const { return list.size(); }

bool Settings::operator==(const Settings &another) const {
  return open_in_editor_command == another.open_in_editor_command &&
         task_history_limit == another.task_history_limit &&
         external_search_folders == another.external_search_folders &&
         documentation_folders == another.documentation_folders &&
         terminals == another.terminals;
}

bool Settings::operator!=(const Settings &another) const {
  return !(*this == another);
}

TerminalListModel::TerminalListModel(QObject *parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}});
}

QVariantList TerminalListModel::GetRow(int i) const {
  return {QString::number(i + 1) + ". " + list[i]};
}

int TerminalListModel::GetRowCount() const { return list.size(); }
