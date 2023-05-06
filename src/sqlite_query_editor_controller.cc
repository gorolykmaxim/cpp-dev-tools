#include "sqlite_query_editor_controller.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>

#include "application.h"
#include "io_task.h"
#include "syntax.h"
#include "theme.h"

#define LOG() qDebug() << "[SqliteQueryEditorController]"

SqliteQueryEditorController::SqliteQueryEditorController(QObject *parent)
    : QObject(parent),
      model(new SqliteTableModel(this)),
      status("Query results with be displayed here"),
      status_color(Theme().kColorSubText),
      formatter(new SyntaxFormatter(this)) {
  Application::Get().view.SetWindowTitle("SQLite Query Editor");
  formatter->DetectLanguageByFile(".sql");
}

static QString GetSelectedQuery(const QString &text, int cursor) {
  if (cursor < 0 || cursor > text.size()) {
    return text;
  }
  QList<TextSectionFormat> strings_and_comments =
      Syntax::FindStringsAndComments(text, "--");
  int start = 0;
  int end = text.size() - 1;
  for (int i = 0; i < text.size(); i++) {
    if (text[i] != ';') {
      continue;
    }
    if (Syntax::SectionsContain(strings_and_comments, i)) {
      continue;
    }
    if (i == cursor - 1 || i >= cursor) {
      end = i - 1;
      break;
    } else {
      start = i + 1;
    }
  }
  return text.sliced(start, end - start + 1);
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
