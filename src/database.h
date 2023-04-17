#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariantList>
#include <QtConcurrent>
#include <functional>

#include "application.h"

class Database {
 public:
  class Transaction {
   public:
    Transaction();
    ~Transaction();
  };
  struct Cmd {
    explicit Cmd(const QString& query, const QVariantList& args = {});

    QString query;
    QVariantList args;
  };

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

  template <typename T>
  static QList<T> ExecQueryAndReadSync(const QString& query,
                                       const std::function<T(QSqlQuery&)>& map,
                                       const QVariantList& args = {}) {
    QFuture<QList<T>> future = QtConcurrent::run(
        &Application::Get().io_thread_pool, [query, map, args] {
          return Database::ExecQueryAndRead<T>(query, map, args);
        });
    while (!future.isResultReadyAt(0)) {
      // If we just call result() - Qt will not just block current thread
      // waiting for the result, it will RUN the specified function on the
      // current thread :) which will crash the app because the Database
      // instance only exists on the IO thread. So instead of relying on Qt
      // to block waiting for result, lets block ourselves with this simple
      // spin-lock.
    };
    return future.result();
  }

  static void ExecQuery(QSqlQuery& sql, const QString& query,
                        const QVariantList& args = {});
  static void ExecCmd(const QString& query, const QVariantList& args = {});
  static void ExecCmdAsync(const QString& query, const QVariantList& args = {});
  static void ExecCmdsAsync(const QList<Cmd>& cmds);
};
