#include "InitSqlDb.hpp"

#include "Db.hpp"
#include "Process.hpp"
#include "Threads.hpp"

#define LOG() qDebug() << "[InitSqlDb]"

InitSqlDb::InitSqlDb() { EXEC_NEXT(Run); }

void InitSqlDb::Run(AppData& app) {
  QPtr<InitSqlDb> self = ProcessSharedPtr(app, this);
  ScheduleIOTask(
      app,
      [&app]() {
        QString home =
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString db_file = home + "/.cpp-dev-tools.db";
        app.db = QSqlDatabase::addDatabase("QSQLITE");
        app.db.setDatabaseName(db_file);
        bool db_opened = app.db.open();
        Q_ASSERT(db_opened);
        LOG() << "Opened database file" << db_file;
        DbTransaction t(app.db);
        ExecDbQuery(app.db,
                    "CREATE TABLE IF NOT EXISTS project("
                    "path VARCHAR PRIMARY KEY, "
                    "is_opened BOOL)");
      },
      [&app, self]() { WakeUpAndExecuteProcess(app, *self); });
  EXEC_NEXT(Noop);
}
