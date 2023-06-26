#include "database.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QUuid>

#include "io_task.h"

#define LOG() qDebug() << "[Database]"

void Database::Initialize() {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#ifdef NDEBUG
  QString db_file = home + "/.cpp-dev-tools.db";
#else
  QString db_file = home + "/.cpp-dev-tools.dev.db";
#endif
  LOG() << "Openning database" << db_file;
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(db_file);
  bool db_opened = db.open();
  Q_ASSERT(db_opened);
  ExecCmd("PRAGMA foreign_keys = ON");
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
      "task_id TEXT, "
      "task_name TEXT, "
      "exit_code INT, "
      "stderr_line_indices TEXT, "
      "output TEXT, "
      "FOREIGN KEY(project_id) REFERENCES project(id) ON DELETE CASCADE)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS editor("
      "id INT PRIMARY KEY DEFAULT 1, "
      "open_command TEXT)");
  ExecCmd("INSERT OR IGNORE INTO editor(open_command) VALUES(?)", {"code {}"});
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS external_search_folder("
      "path TEXT PRIMARY KEY)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS split_view_state("
      "id TEXT, "
      "virtual_width INT, "
      "virtual_height INT, "
      "virtual_x INT, "
      "virtual_y INT, "
      "state BLOB, "
      "PRIMARY KEY(id, virtual_width, virtual_height, virtual_x, virtual_y))");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS task_context("
      "id INT PRIMARY KEY DEFAULT 1,"
      "history_limit INT)");
  ExecCmd("INSERT OR IGNORE INTO task_context(history_limit) VALUES(10)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS documentation_folder("
      "path TEXT PRIMARY KEY)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS database_file("
      "id BLOB NOT NULL,"
      "project_id BLOB NOT NULL,"
      "path TEXT NOT NULL,"
      "is_selected BOOL DEFAULT FALSE,"
      "editor_query TEXT DEFAULT '',"
      "PRIMARY KEY(id, project_id),"
      "FOREIGN KEY(project_id) REFERENCES project(id) ON DELETE CASCADE)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS find_in_files_context("
      "project_id BLOB PRIMARY KEY, "
      "search_term TEXT, "
      "match_case BOOL DEFAULT FALSE, "
      "match_whole_word BOOL DEFAULT FALSE, "
      "regexp BOOL DEFAULT FALSE, "
      "include_external_search_folders BOOL DEFAULT FALSE, "
      "exclude_git_ignored_files BOOL DEFAULT TRUE, "
      "expanded BOOL DEFAULT FALSE, "
      "files_to_include TEXT, "
      "files_to_exclude TEXT, "
      "FOREIGN KEY(project_id) REFERENCES project(id) ON DELETE CASCADE)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS terminal("
      "name TEXT PRIMARY KEY, "
      "priority INT NOT NULL)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS git_diff_context("
      "id INT PRIMARY KEY DEFAULT 1, "
      "side_by_side_view BOOL NOT NULL)");
  ExecCmd(
      "INSERT OR IGNORE INTO git_diff_context(side_by_side_view) "
      "VALUES(TRUE)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS git_log_context("
      "project_id BLOB PRIMARY KEY, "
      "branch_or_file TEXT, "
      "search_term TEXT, "
      "FOREIGN KEY(project_id) REFERENCES project(id) ON DELETE CASCADE)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS git_file_diff_context("
      "project_id BLOB PRIMARY KEY, "
      "branches_to_compare TEXT, "
      "file_path TEXT, "
      "FOREIGN KEY(project_id) REFERENCES project(id) ON DELETE CASCADE)");
  ExecCmd(
      "CREATE TABLE IF NOT EXISTS git_commit_context("
      "project_id BLOB PRIMARY KEY, "
      "message TEXT, "
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

void Database::ExecCmdsAsync(const QList<Cmd> &cmds) {
  (void)QtConcurrent::run(&Application::Get().io_thread_pool, [cmds] {
    Transaction t;
    for (const Cmd &cmd : cmds) {
      ExecCmd(cmd.query, cmd.args);
    }
  });
}

QString Database::ReadStringFromSql(QSqlQuery &sql) {
  return sql.value(0).toString();
}

int Database::ReadIntFromSql(QSqlQuery &sql) { return sql.value(0).toInt(); }

static QVariantList ReadFromSql(QSqlQuery &sql) {
  QVariantList res;
  for (int i = 0; i < sql.record().count(); i++) {
    res.append(sql.value(i));
  }
  return res;
}

void Database::LoadState(QObject *ctx, const QString &query,
                         const QVariantList &args,
                         std::function<void(QVariantList)> &&cb) {
  IoTask::Run<QList<QVariantList>>(
      ctx,
      [query, args] {
        return Database::ExecQueryAndRead<QVariantList>(query, ReadFromSql,
                                                        args);
      },
      [cb = std::move(cb)](QList<QVariantList> results) {
        if (results.isEmpty()) {
          cb({});
        } else {
          cb(results.constFirst());
        }
      });
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

Database::Cmd::Cmd(const QString &query, const QVariantList &args)
    : query(query), args(args) {}
