#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariantList>
#include <functional>

class Database {
 public:
  static void Initialize();

  template <typename T>
  static QList<T> ExecQueryAndRead(const QString& query,
                                   const std::function<T(QSqlQuery&)>& map,
                                   const QVariantList& args = {}) {
    QSqlQuery sql(QSqlDatabase::database());
    ExecQuery(sql, query, args);
    QList<T> result;
    while (sql.next()) {
      result.append(map(sql));
    }
    return result;
  }

  static void ExecQuery(QSqlQuery& sql, const QString& query,
                        const QVariantList& args = {});
  static void ExecCmd(const QString& query, const QVariantList& args = {});

  class Transaction {
   public:
    Transaction();
    ~Transaction();
  };
};
