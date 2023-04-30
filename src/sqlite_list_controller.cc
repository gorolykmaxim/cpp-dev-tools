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
  bool a_selected = a.path == app.sqlite.GetSelectedFile();
  bool b_selected = b.path == app.sqlite.GetSelectedFile();
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
  if (app.sqlite.GetSelectedFile() == file.path) {
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
    : QObject(parent), databases(new SqliteFileListModel(this)) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("SQLite Databases");
  QUuid project_id = app.project.GetCurrentProject().id;
  IoTask::Run<QList<SqliteFile>>(
      this,
      [project_id] {
        QList<QString> paths = Database::ExecQueryAndRead<QString>(
            "SELECT path FROM database_file WHERE project_id=?",
            &Database::ReadStringFromSql, {project_id});
        QList<SqliteFile> files;
        for (const QString& path : paths) {
          SqliteFile file;
          file.path = path;
          file.is_missing_on_disk = !QFile::exists(path);
          files.append(file);
        }
        return files;
      },
      [this](QList<SqliteFile> files) {
        databases->list = files;
        databases->SortAndLoad();
      });
}

void SqliteListController::selectDatabase(int i) {
  const SqliteFile& file = databases->list[i];
  if (file.is_missing_on_disk) {
    return;
  }
  LOG() << "Selecting database" << file.path;
  Application::Get().sqlite.SetSelectedFile(file.path);
  databases->SortAndLoad();
}

void SqliteListController::removeDatabase(int i) {
  QString path = databases->list[i].path;
  LOG() << "Removing database" << path;
  databases->list.remove(i);
  Application& app = Application::Get();
  QUuid project_id = app.project.GetCurrentProject().id;
  Database::ExecCmdAsync(
      "DELETE FROM database_file WHERE project_id=? AND path=?",
      {project_id, path});
  if (path == app.sqlite.GetSelectedFile()) {
    QString new_database_to_select;
    for (const SqliteFile& file : databases->list) {
      if (!file.is_missing_on_disk) {
        new_database_to_select = file.path;
        break;
      }
    }
    app.sqlite.SetSelectedFile(new_database_to_select);
  }
  databases->SortAndLoad();
}

SqliteFile::SqliteFile(const QString& path, bool is_missing_on_disk)
    : path(path), is_missing_on_disk(is_missing_on_disk) {}
