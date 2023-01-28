#pragma once

#include "Lib.hpp"

class DbTransaction {
 public:
  explicit DbTransaction(QSqlDatabase& db);
  ~DbTransaction();

 private:
  QSqlDatabase& db;
};

void ExecDbQuery(QSqlDatabase& db, const QString& query);
