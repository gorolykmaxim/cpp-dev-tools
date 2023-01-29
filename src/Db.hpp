#pragma once

#include "AppData.hpp"

class DbTransaction {
 public:
  explicit DbTransaction(QSqlDatabase& db);
  ~DbTransaction();

 private:
  QSqlDatabase& db;
};

void ExecDbQuery(QSqlQuery& sql_query, const QString& query,
                 const QVariantList& args = {});

void ExecDbCmd(QSqlDatabase& db, const QString& query,
               const QVariantList& args = {});
void ExecDbCmdOnIOThread(AppData& app, const QString& query,
                         const QVariantList& args = {});

template <typename T>
QList<T> ExecDbQueryAndRead(QSqlDatabase& db, const QString& query,
                            const std::function<T(QSqlQuery&)>& map,
                            const QVariantList& args = {}) {
  QSqlQuery sql(db);
  ExecDbQuery(sql, query, args);
  QList<T> result;
  while (sql.next()) {
    result.append(map(sql));
  }
  return result;
}

int ExecDbQueryCount(QSqlDatabase& db, const QString& query,
                     const QVariantList& args = {});
Project ReadProjectFromSql(QSqlQuery& sql);
