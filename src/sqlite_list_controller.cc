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
    : QVariantListModel(parent) {
  SetRoleNames({{0, "idx"},
                {1, "title"},
                {2, "subTitle"},
                {3, "existsOnDisk"},
                {4, "titleColor"},
                {5, "icon"},
                {6, "iconColor"}});
  searchable_roles = {1, 2};
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
    title_color = kTheme.kColorSubText;
    icon.icon = "not_interested";
    icon.color = title_color;
    title = "[Not Found] " + title;
  } else {
    icon.icon = "storage";
  }
  return {i,           title,     file.path, !file.is_missing_on_disk,
          title_color, icon.icon, icon.color};
}

int SqliteFileListModel::GetRowCount() const { return list.size(); }

SqliteListController::SqliteListController(QObject* parent)
    : QObject(parent), databases(new SqliteFileListModel(this)) {}

QUuid SqliteListController::GetSelectedDatabaseFileId() const {
  return selected.id;
}

bool SqliteListController::IsDatabaseSelected() const {
  return !selected.IsNull();
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
  if (selected.IsNull() || selected.is_missing_on_disk) {
    return;
  }
  LOG() << "Selecting database" << selected.path << "globally";
  Application::Get().sqlite.SetSelectedFile(selected);
  databases->SortAndLoad();
}

void SqliteListController::removeSelectedDatabase() {
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

void SqliteListController::selectDatabase(int i) {
  selected = databases->list[i];
  LOG() << "Selecting database" << selected.path;
  emit selectedDatabaseChanged();
}
