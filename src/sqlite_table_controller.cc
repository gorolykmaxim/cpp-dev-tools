#include "sqlite_table_controller.h"

#include "application.h"
#include "io_task.h"
#include "theme.h"

#define LOG() qDebug() << "[SqliteTableController]"

SqliteTableController::SqliteTableController(QObject* parent)
    : QObject(parent),
      tables(new TableListModel(this)),
      table(new SqliteTableModel(this)) {}

QString SqliteTableController::GetLimit() const { return query.limit; }

void SqliteTableController::displayTableList() {
  Application& app = Application::Get();
  app.view.SetWindowTitle("SQLite Tables");
  SqliteFile file = app.sqlite.GetSelectedFile();
  LOG() << "Fetching list of tables from" << file.path;
  SetStatus("Looking for tables...");
  IoTask::Run<std::pair<QStringList, QString>>(
      this,
      [file] {
        QStringList results;
        QSqlDatabase db =
            QSqlDatabase::database(SqliteSystem::kConnectionName, false);
        QString error;
        if (!SqliteSystem::OpenIfExistsSync(db, error)) {
          return std::make_pair(results, error);
        }
        QSqlQuery sql(db);
        sql.exec(
            "SELECT name FROM sqlite_schema WHERE type='table' AND name NOT "
            "LIKE 'sqlite_%' ORDER BY name");
        while (sql.next()) {
          results.append(sql.value(0).toString());
        }
        db.close();
        return std::make_pair(results, error);
      },
      [this](std::pair<QStringList, QString> results) {
        tables->list.clear();
        if (!results.second.isEmpty()) {
          SetStatus(results.second, "red");
        } else {
          tables->list = results.first;
          SetStatus(results.first.isEmpty() ? "No Tables Found" : "");
        }
        tables->Load(-1);
      });
}

void SqliteTableController::displaySelectedTable() {
  QString table_name = tables->GetSelected();
  if (table_name.isEmpty()) {
    return;
  }
  LOG() << "Table" << table_name << "selected";
  Application::Get().view.SetWindowTitle("SQLite Table '" + table_name + '\'');
  query = Query();
  emit displayTableView();
  load();
}

void SqliteTableController::load() {
  SetStatus("Loading table data...");
  QString sql = "SELECT * FROM " + tables->GetSelected();
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

TableListModel::TableListModel(QObject* parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "icon"}});
  searchable_roles = {0};
}

QString TableListModel::GetSelected() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? "" : list[i];
}

QVariantList TableListModel::GetRow(int i) const {
  return {list[i], "web_asset"};
}

int TableListModel::GetRowCount() const { return list.size(); }
