#pragma once

#include "Lib.hpp"

class DbTransaction {
 public:
  explicit DbTransaction(QSqlDatabase& db);
  ~DbTransaction();

 private:
  QSqlDatabase& db;
};

void ExecDbCmd(QSqlDatabase& db, const QString& cmd);
