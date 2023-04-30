#include "sqlite_system.h"

#include <QDebug>

#include "database.h"
#include "io_task.h"
#include "path.h"

#define LOG() qDebug() << "[SqliteSystem]"

void SqliteSystem::InitializeSelectedFile() {
  QUuid project_id = Application::Get().project.GetCurrentProject().id;
  IoTask::Run<QList<QString>>(
      this,
      [project_id] {
        LOG() << "Initializing default connection";
        QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
        return Database::ExecQueryAndRead<QString>(
            "SELECT path FROM database_file WHERE is_selected=true AND "
            "project_id=?",
            &Database::ReadStringFromSql, {project_id});
      },
      [this](QList<QString> paths) {
        if (!paths.isEmpty()) {
          SetSelectedFile(paths[0]);
        }
      });
}

void SqliteSystem::SetSelectedFile(const QString &file) {
  LOG() << "Selecting database file:" << file;
  selected_file = file;
  emit selectedFileChanged();
  if (file.isEmpty()) {
    return;
  }
  QUuid project_id = Application::Get().project.GetCurrentProject().id;
  IoTask::Run(
      this,
      [file, project_id] {
        Database::Transaction t;
        Database::ExecCmd(
            "UPDATE database_file SET is_selected=false WHERE project_id=?",
            {project_id});
        Database::ExecCmd(
            "UPDATE database_file SET is_selected=true WHERE path=? AND "
            "project_id=?",
            {file, project_id});
        QSqlDatabase db = QSqlDatabase::database(kConnectionName, false);
        db.setDatabaseName(file);
      },
      [] {});
}

QString SqliteSystem::GetSelectedFile() const { return selected_file; }

QString SqliteSystem::GetSelectedFileName() const {
  return Path::GetFileName(GetSelectedFile());
}
