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

void ExecDbCmd(QSqlDatabase& db, const QString& cmd) {
  LOG() << "Executing command:" << cmd;
  QSqlQuery sql_cmd(db);
  bool cmd_executed = sql_cmd.exec(cmd);
  if (!cmd_executed) {
    LOG() << "Last sql command failed:" << sql_cmd.lastError().text();
  }
  Q_ASSERT(cmd_executed);
}
