#include "sqlite_system.h"

#include <QDebug>

#include "database.h"
#include "io_task.h"
#include "path.h"

#define LOG() qDebug() << "[SqliteSystem]"

SqliteFile SqliteSystem::ReadFromSql(QSqlQuery& sql) {
  SqliteFile file;
  file.id = sql.value(0).toUuid();
  file.path = sql.value(1).toString();
  return file;
}

void SqliteSystem::InitializeSelectedFile() {
  QUuid project_id = Application::Get().project.GetCurrentProject().id;
  IoTask::Run<QList<SqliteFile>>(
      this,
      [project_id] {
        LOG() << "Initializing default connection";
        QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
        return Database::ExecQueryAndRead<SqliteFile>(
            "SELECT id, path FROM database_file WHERE is_selected=true AND "
            "project_id=?",
            &ReadFromSql, {project_id});
      },
      [this](QList<SqliteFile> files) {
        if (!files.isEmpty()) {
          SetSelectedFile(files[0]);
        }
      });
}

void SqliteSystem::SetSelectedFile(const SqliteFile& file) {
  LOG() << "Selecting database file:" << file.path;
  selected_file = file;
  emit selectedFileChanged();
  if (file.IsNull()) {
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
            "UPDATE database_file SET is_selected=true WHERE id=? AND "
            "project_id=?",
            {file.id, project_id});
        QSqlDatabase db = QSqlDatabase::database(kConnectionName, false);
        db.setDatabaseName(file.path);
      },
      [] {});
}

SqliteFile SqliteSystem::GetSelectedFile() const { return selected_file; }

QString SqliteSystem::GetSelectedFileName() const {
  return Path::GetFileName(selected_file.path);
}

bool SqliteFile::IsNull() const { return id.isNull(); }

bool SqliteFile::operator==(const SqliteFile& another) const {
  return id == another.id;
}

bool SqliteFile::operator!=(const SqliteFile& another) const {
  return !(*this == another);
}