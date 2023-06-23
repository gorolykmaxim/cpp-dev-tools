#include "sqlite_list_controller.h"

#include <QSqlError>

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "path.h"
#include "theme.h"
#include "ui_icon.h"

#define LOG() qDebug() << "[SqliteListController]"

SqliteFileListModel::SqliteFileListModel(QObject* parent)
    : TextListModel(parent) {
  SetRoleNames({{0, "title"},
                {1, "subTitle"},
                {2, "existsOnDisk"},
                {3, "titleColor"},
                {4, "icon"},
                {5, "iconColor"}});
  searchable_roles = {0, 1};
  SetEmptyListPlaceholder(
      "Open databases by clicking on 'Add Database' button");
}

static bool Compare(const SqliteFile& a, const SqliteFile& b) {
  Application& app = Application::Get();
  SqliteFile selected = app.sqlite.GetSelectedFile();
  bool a_selected = a == selected;
  bool b_selected = b == selected;
  if (a_selected != b_selected) {
    return a_selected > b_selected;
  } else {
    return a.path < b.path;
  }
}

void SqliteFileListModel::SortAndLoad() {
  std::sort(list.begin(), list.end(), Compare);
  Load();
}

SqliteFile SqliteFileListModel::GetSelected() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? SqliteFile() : list[i];
}

QVariantList SqliteFileListModel::GetRow(int i) const {
  static const Theme kTheme;
  Application& app = Application::Get();
  const SqliteFile& file = list[i];
  QString title_color;
  UiIcon icon;
  QString title = Path::GetFileName(file.path);
  if (app.sqlite.GetSelectedFile() == file) {
    title = "[Selected] " + title;
  }
  if (file.is_missing_on_disk) {
    title_color = kTheme.kColorPlaceholder;
    icon.icon = "not_interested";
    icon.color = title_color;
    title = "[Not Found] " + title;
  } else {
    icon.icon = "storage";
  }
  return {title,       file.path, !file.is_missing_on_disk,
          title_color, icon.icon, icon.color};
}

int SqliteFileListModel::GetRowCount() const { return list.size(); }

SqliteListController::SqliteListController(QObject* parent)
    : QObject(parent), databases(new SqliteFileListModel(this)) {
  QObject::connect(databases, &TextListModel::selectedItemChanged, this,
                   [this] { emit selectedDatabaseChanged(); });
}

QUuid SqliteListController::GetSelectedDatabaseFileId() const {
  return databases->GetSelected().id;
}

bool SqliteListController::IsDatabaseSelected() const {
  return !databases->GetSelected().IsNull();
}

void SqliteListController::displayDatabaseList() {
  Application& app = Application::Get();
  app.view.SetWindowTitle("SQLite Databases");
  QUuid project_id = app.project.GetCurrentProject().id;
  IoTask::Run<QList<SqliteFile>>(
      this,
      [project_id] {
        LOG() << "Loading database files from database";
        QList<SqliteFile> files = Database::ExecQueryAndRead<SqliteFile>(
            "SELECT id, path FROM database_file WHERE project_id=?",
            &SqliteSystem::ReadFromSql, {project_id});
        for (SqliteFile& file : files) {
          file.is_missing_on_disk = !QFile::exists(file.path);
        }
        return files;
      },
      [this](QList<SqliteFile> files) {
        databases->list = files;
        databases->SortAndLoad();
      });
}

void SqliteListController::useSelectedDatabase() {
  SqliteFile selected = databases->GetSelected();
  if (selected.IsNull() || selected.is_missing_on_disk) {
    return;
  }
  LOG() << "Selecting database" << selected.path << "globally";
  Application::Get().sqlite.SetSelectedFile(selected);
  databases->SortAndLoad();
}

void SqliteListController::removeSelectedDatabase() {
  SqliteFile selected = databases->GetSelected();
  LOG() << "Removing database" << selected.path;
  databases->list.removeAll(selected);
  Application& app = Application::Get();
  QUuid project_id = app.project.GetCurrentProject().id;
  Database::ExecCmdAsync(
      "DELETE FROM database_file WHERE project_id=? AND id=?",
      {project_id, selected.id});
  if (selected == app.sqlite.GetSelectedFile()) {
    selected = SqliteFile();
    for (const SqliteFile& file : databases->list) {
      if (!file.is_missing_on_disk) {
        selected = file;
        break;
      }
    }
    app.sqlite.SetSelectedFile(selected);
  }
  databases->SortAndLoad();
}
