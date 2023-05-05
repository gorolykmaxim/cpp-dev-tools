#include "sqlite_query_editor_controller.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>

#include "io_task.h"
#include "sqlite_system.h"
#include "theme.h"

#define LOG() qDebug() << "[SqliteQueryEditorController]"

SqliteQueryEditorController::SqliteQueryEditorController(QObject *parent)
    : QObject(parent),
      model(new SqliteTableModel(this)),
      status("Query results with be displayed here"),
      status_color(Theme().kColorSubText) {}

static bool PrevCharEqual(const QString &text, int i, QChar c) {
  return i > 1 && text[i] == c;
}

static QString GetSelectedQuery(const QString &text, int cursor) {
  // TODO: try to simplify
  if (cursor < 0 || cursor > text.size()) {
    return text;
  }
  int start = 0;
  int end = text.size() - 1;
  bool is_quote = false;
  bool is_double_quote = false;
  bool is_comment = false;
  for (int i = 0; i < text.size(); i++) {
    if (text[i] == '\'' && !PrevCharEqual(text, i, '\\') && !is_double_quote &&
        !is_comment) {
      is_quote = !is_quote;
    } else if (text[i] == '"' && !PrevCharEqual(text, i, '\\') && !is_quote &&
               !is_comment) {
      is_double_quote = !is_double_quote;
    } else if (text[i] == '-' && PrevCharEqual(text, i, '-') && !is_quote &&
               !is_double_quote) {
      is_comment = true;
    } else if (text[i] == '\n') {
      is_quote = false;
      is_double_quote = false;
      is_comment = false;
    }
    if (text[i] == ';' && !is_quote && !is_double_quote && !is_comment) {
      if (i == cursor - 1 || i >= cursor) {
        end = i;
        break;
      } else {
        start = i + 1;
      }
    }
  }
  return text.sliced(start, end - start);
}

void SqliteQueryEditorController::executeQuery(const QString &text,
                                               int cursor) {
  QString query = GetSelectedQuery(text, cursor);
  LOG() << "Executing query" << query;
  SetStatus("Running query...");
  IoTask::Run<SqliteQueryResult>(
      this,
      [query] {
        SqliteQueryResult result;
        QSqlDatabase db = QSqlDatabase::database(SqliteSystem::kConnectionName);
        if (!db.open()) {
          result.error =
              "Failed to open database: " + db.lastError().databaseText();
          return result;
        }
        QSqlQuery sql(db);
        if (sql.exec(query)) {
          result.is_select = sql.isSelect();
          if (result.is_select) {
            while (sql.next()) {
              if (result.columns.isEmpty()) {
                QSqlRecord record = sql.record();
                for (int i = 0; i < record.count(); i++) {
                  result.columns.append(record.fieldName(i));
                }
              }
              QVariantList row;
              for (int i = 0; i < result.columns.size(); i++) {
                row.append(sql.value(i));
              }
              result.rows.append(row);
            }
          } else {
            result.rows_affected = sql.numRowsAffected();
          }
        } else {
          result.error =
              "Failed to execute query: " + sql.lastError().databaseText();
        }
        db.close();
        return result;
      },
      [this](SqliteQueryResult result) {
        if (!result.error.isEmpty()) {
          SetStatus(result.error, "red");
        } else if (result.is_select) {
          model->SetTable(result.columns, result.rows);
          SetStatus("");
        } else {
          SetStatus("Query executed. Rows affected: " +
                    QString::number(result.rows_affected));
        }
      });
}

void SqliteQueryEditorController::SetStatus(const QString &status,
                                            const QString &color) {
  static const Theme kTheme;
  this->status = status;
  this->status_color = color.isEmpty() ? kTheme.kColorText : color;
  emit statusChanged();
}
