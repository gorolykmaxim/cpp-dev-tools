#include "sqlite_table_controller.h"

#include "application.h"
#include "io_task.h"
#include "theme.h"

#define LOG() qDebug() << "[SqliteTableController]"

SqliteTableController::SqliteTableController(QObject* parent)
    : QObject(parent),
      tables(new SimpleQVariantListModel(
          this, {{0, "idx"}, {1, "title"}, {2, "icon"}, {3, "isSelected"}},
          {1})),
      table(new SqliteTableModel(this)) {}

bool SqliteTableController::AreTablesFound() const {
  return !tables->list.isEmpty();
}

QString SqliteTableController::GetLimit() const { return query.limit; }

void SqliteTableController::displayTableList() {
  Application& app = Application::Get();
  app.view.SetWindowTitle("SQLite Tables");
  SqliteFile file = app.sqlite.GetSelectedFile();
  LOG() << "Fetching list of tables from" << file.path;
  IoTask::Run<QStringList>(
      this,
      [file] {
        QStringList results;
        QSqlDatabase db = QSqlDatabase::database(SqliteSystem::kConnectionName);
        db.open();
        if (!db.open()) {
          return results;
        }
        QSqlQuery sql(db);
        sql.exec(
            "SELECT name FROM sqlite_schema WHERE type='table' AND name NOT "
            "LIKE 'sqlite_%' ORDER BY name");
        while (sql.next()) {
          results.append(sql.value(0).toString());
        }
        db.close();
        return results;
      },
      [this](QStringList results) {
        tables->list.clear();
        for (int i = 0; i < results.size(); i++) {
          const QString& table = results[i];
          tables->list.append({i, table, "web_asset", table == table_name});
        }
        tables->Load();
        emit tablesChanged();
      });
}

void SqliteTableController::displayTable(int i) {
  table_name = tables->list[i][1].toString();
  LOG() << "Table" << table_name << "selected";
  Application::Get().view.SetWindowTitle("SQLite Table '" + table_name + '\'');
  query = Query();
  emit displayTableView();
  load();
}

void SqliteTableController::load() {
  SetStatus("Loading...");
  QString sql = "SELECT * FROM " + table_name;
  if (!query.where.isEmpty()) {
    sql += " WHERE " + query.where;
  }
  if (!query.order_by.isEmpty()) {
    sql += " ORDER BY " + query.order_by;
  }
  if (!query.limit.isEmpty()) {
    sql += " LIMIT " + query.limit;
  }
  if (!query.offset.isEmpty()) {
    sql += " OFFSET " + query.offset;
  }
  SqliteSystem::ExecuteQuery(this, sql, [this](SqliteQueryResult result) {
    if (!result.error.isEmpty()) {
      SetStatus(result.error, "red");
    } else {
      table->SetTable(result.columns, result.rows);
      SetStatus("");
    }
  });
}

void SqliteTableController::setLimit(const QString& value) {
  query.limit = value;
}

void SqliteTableController::setOffset(const QString& value) {
  query.offset = value;
}

void SqliteTableController::setOrderBy(const QString& value) {
  query.order_by = value;
}

void SqliteTableController::setWhere(const QString& value) {
  query.where = value;
}

void SqliteTableController::SetStatus(const QString& status,
                                      const QString& color) {
  static const Theme kTheme;
  this->status = status;
  this->status_color = color.isEmpty() ? kTheme.kColorText : color;
  emit statusChanged();
}
