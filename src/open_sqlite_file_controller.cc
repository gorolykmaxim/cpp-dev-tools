#include "open_sqlite_file_controller.h"

#include <QSqlError>

#include "application.h"
#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[OpenSqliteFileController]"

static QString Validate(QSqlDatabase& db) {
  if (!db.open()) {
    return db.lastError().text();
  }
  QSqlQuery sql("SELECT * FROM sqlite_schema", db);
  if (!sql.exec()) {
    return "Specified file is not a SQLite database";
  }
  return "";
}

OpenSqliteFileController::OpenSqliteFileController(QObject* parent)
    : QObject(parent) {
  Application::Get().view.SetWindowTitle("Open SQLite Database");
}

void OpenSqliteFileController::openDatabase(const QString& path) {
  LOG() << "Opening database" << path;
  Application& app = Application::Get();
  QUuid project_id = app.project.GetCurrentProject().id;
  IoTask::Run<std::pair<SqliteFile, QString>>(
      this,
      [path, project_id] {
        SqliteFile file;
        QString error;
        QList<SqliteFile> existing = Database::ExecQueryAndRead<SqliteFile>(
            "SELECT id, path FROM database_file WHERE path=? AND project_id=?",
            &SqliteSystem::ReadFromSql, {path, project_id});
        if (!existing.isEmpty()) {
          file = existing[0];
        } else {
          file.id = QUuid::createUuid();
          file.path = path;
          QSqlDatabase db =
              QSqlDatabase::database(SqliteSystem::kConnectionName);
          db.setDatabaseName(path);
          error = Validate(db);
          if (error.isEmpty()) {
            Database::ExecCmd(
                "INSERT INTO database_file(id, path, project_id) "
                "VALUES(?, ?, ?)",
                {file.id, file.path, project_id});
          }
        }
        return std::make_pair(file, error);
      },
      [this, &app](std::pair<SqliteFile, QString> result) {
        auto [file, error] = result;
        if (error.isEmpty()) {
          app.sqlite.SetSelectedFile(file);
          emit databaseOpened();
        } else {
          app.view.DisplayAlertDialog(
              "Failed to open SQLite file",
              "Failed to open " + file.path + ": " + error, true);
        }
      });
}
