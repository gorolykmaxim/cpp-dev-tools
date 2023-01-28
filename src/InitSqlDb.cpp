#include "InitSqlDb.hpp"

#include "Db.hpp"
#include "Process.hpp"
#include "Threads.hpp"

#define LOG() qDebug() << "[InitSqlDb]"

InitSqlDb::InitSqlDb() { EXEC_NEXT(Run); }

static void OpenDb(QSqlDatabase& db) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString db_file = home + "/.cpp-dev-tools.db";
  db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(db_file);
  bool db_opened = db.open();
  Q_ASSERT(db_opened);
  LOG() << "Opened database file" << db_file;
}

static void InitDbSchema(QSqlDatabase& db) {
  DbTransaction t(db);
  QSqlQuery query(db);
  ExecDbQuery(query,
              "CREATE TABLE IF NOT EXISTS project(id BLOB PRIMARY KEY, path "
              "TEXT, is_opened BOOL, last_open_time DATETIME)");
}

void InitSqlDb::Run(AppData& app) {
  QPtr<InitSqlDb> self = ProcessSharedPtr(app, this);
  ScheduleIOTask(
      app,
      [&app]() {
        OpenDb(app.db);
        InitDbSchema(app.db);
      },
      [&app, self]() { WakeUpAndExecuteProcess(app, *self); });
  EXEC_NEXT(Noop);
}
