#include "Db.hpp"

#include "Threads.hpp"

#define LOG() qDebug() << "[Db]"

DbTransaction::DbTransaction(QSqlDatabase& db) : db(db) {
  LOG() << "Begin transaction";
  bool transaction_began = db.transaction();
  Q_ASSERT(transaction_began);
}

DbTransaction::~DbTransaction() {
  LOG() << "Commit transaction";
  bool transaction_committed = db.commit();
  Q_ASSERT(transaction_committed);
}

void ExecDbQuery(QSqlQuery& sql_query, const QString& query,
                 const QVariantList& args) {
  LOG() << "Executing query:" << query;
  bool query_executed = false;
  if (args.isEmpty()) {
    query_executed = sql_query.exec(query);
  } else {
    sql_query.prepare(query);
    for (int i = 0; i < args.size(); i++) {
      QVariant arg = args[i];
      if (arg.userType() == QMetaType::QUuid) {
        QString str = arg.toUuid().toString(QUuid::StringFormat::WithoutBraces);
        sql_query.bindValue(i, str);
      } else {
        sql_query.bindValue(i, arg);
      }
    }
    query_executed = sql_query.exec();
  }
  if (!query_executed) {
    LOG() << "Last query failed:" << sql_query.lastError().text();
  }
  Q_ASSERT(query_executed);
}

void ExecDbCmd(QSqlDatabase& db, const QString& query,
               const QVariantList& args) {
  QSqlQuery sql(db);
  ExecDbQuery(sql, query, args);
}

void ExecDbCmdOnIOThread(AppData& app, const QString& query,
                         const QVariantList& args) {
  ScheduleIOTask(
      app, [&app, query, args]() { ExecDbCmd(app.db, query, args); }, []() {});
}

static int ReadCountFromSql(QSqlQuery& sql) { return sql.value(0).toInt(); }

int ExecDbQueryCount(QSqlDatabase& db, const QString& query,
                     const QVariantList& args) {
  return ExecDbQueryAndRead<int>(db, query, ReadCountFromSql, args).first();
}

Project ReadProjectFromSql(QSqlQuery& sql) {
  Project project;
  project.id = sql.value(0).toUuid();
  project.path = sql.value(1).toString();
  project.is_opened = sql.value(2).toBool();
  project.last_open_time = sql.value(3).toDateTime();
  return project;
}
