#include "Db.hpp"

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

void ExecDbQuery(QSqlDatabase& db, const QString& query) {
  LOG() << "Executing query" << query;
  QSqlQuery sql_query(db);
  bool query_executed = sql_query.exec(query);
  Q_ASSERT(query_executed);
}
