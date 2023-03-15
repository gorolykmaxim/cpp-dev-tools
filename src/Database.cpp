#include "Database.hpp"

#include <QDebug>
#include <QSqlError>
#include <QStandardPaths>
#include <QUuid>
#include <QtConcurrent>

#include "Application.hpp"

#define LOG() qDebug() << "[Database]"

void Database::Initialize() {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString db_file = home + "/.cpp-dev-tools.db";
  LOG() << "Openning database" << db_file;
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(db_file);
  bool db_opened = db.open();
  Q_ASSERT(db_opened);
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS project(id BLOB PRIMARY KEY, path TEXT, "
      "is_opened BOOL, last_open_time DATETIME)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS window_dimensions(screen_width INT, "
      "screen_height INT, width INT, height INT, x INT, y INT, is_maximized "
      "BOOL, PRIMARY KEY(screen_width, screen_height))");
}

void Database::ExecQuery(QSqlQuery &sql, const QString &query,
                         const QVariantList &args) {
  LOG() << "Executing query:" << query;
  bool query_executed = false;
  if (args.isEmpty()) {
    query_executed = sql.exec(query);
  } else {
    sql.prepare(query);
    for (int i = 0; i < args.size(); i++) {
      QVariant arg = args[i];
      if (arg.userType() == QMetaType::QUuid) {
        QString str = arg.toUuid().toString(QUuid::StringFormat::WithoutBraces);
        sql.bindValue(i, str);
      } else {
        sql.bindValue(i, arg);
      }
    }
    query_executed = sql.exec();
  }
  if (!query_executed) {
    LOG() << "Last query failed:" << sql.lastError().text();
  }
  Q_ASSERT(query_executed);
}

void Database::ExecCmd(const QString &query, const QVariantList &args) {
  QSqlQuery sql(QSqlDatabase::database());
  ExecQuery(sql, query, args);
}

void Database::ExecCmdAsync(const QString &query, const QVariantList &args) {
  (void)QtConcurrent::run(&Application::Get().io_thread_pool,
                          [query, args] { ExecCmd(query, args); });
}

Database::Transaction::Transaction() {
  LOG() << "Begin transaction";
  bool transaction_began = QSqlDatabase::database().transaction();
  Q_ASSERT(transaction_began);
}

Database::Transaction::~Transaction() {
  LOG() << "Commit transaction";
  bool transaction_committed = QSqlDatabase::database().commit();
  Q_ASSERT(transaction_committed);
}
