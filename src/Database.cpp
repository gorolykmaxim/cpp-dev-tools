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
      "CREATE TABLE IF NOT EXISTS project("
      "id BLOB PRIMARY KEY, "
      "path TEXT, "
      "is_opened BOOL, "
      "last_open_time DATETIME)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS window_dimensions("
      "virtual_width INT, "
      "virtual_height INT, "
      "virtual_x INT, "
      "virtual_y INT, "
      "width INT, "
      "height INT, "
      "x INT, "
      "y INT, "
      "is_maximized BOOL, "
      "PRIMARY KEY(virtual_width, virtual_height, virtual_x, virtual_y))");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS task_execution("
      "id BLOB PRIMARY KEY, "
      "project_id BLOB, "
      "start_time DATETIME, "
      "command TEXT, "
      "exit_code INT, "
      "stderr_line_indices TEXT, "
      "output TEXT, "
      "FOREIGN KEY(project_id) REFERENCES project(id) ON DELETE CASCADE)");
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
