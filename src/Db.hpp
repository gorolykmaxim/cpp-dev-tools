#pragma once

#include "Lib.hpp"

class DbTransaction {
 public:
  explicit DbTransaction(QSqlDatabase& db);
  ~DbTransaction();

 private:
  QSqlDatabase& db;
};

void ExecDbQuery(QSqlQuery& sql_query, const QString& query,
                 const QVariantList& args = {});
