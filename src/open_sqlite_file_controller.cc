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
  IoTask::Run<QString>(
      this,
      [path, project_id] {
        QSqlDatabase db = QSqlDatabase::database(SqliteSystem::kConnectionName);
        db.setDatabaseName(path);
        QString error = Validate(db);
        if (error.isEmpty()) {
          Database::ExecCmd(
              "INSERT OR IGNORE INTO database_file(path, project_id) "
              "VALUES(?, ?)",
              {path, project_id});
        }
        return error;
      },
      [this, path, &app](QString error) {
        if (error.isEmpty()) {
          app.sqlite.SetSelectedFile(path);
          emit databaseOpened();
        } else {
          app.view.DisplayAlertDialog("Failed to open SQLite file",
                                      "Failed to open " + path + ": " + error,
                                      true);
        }
      });
}
