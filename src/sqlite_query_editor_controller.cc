#include "sqlite_query_editor_controller.h"

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "syntax.h"
#include "theme.h"

#define LOG() qDebug() << "[SqliteQueryEditorController]"

SqliteQueryEditorController::SqliteQueryEditorController(QObject *parent)
    : QObject(parent),
      model(new SqliteTableModel(this)),
      formatter(new SyntaxFormatter(this)) {
  Application &app = Application::Get();
  app.view.SetWindowTitle("SQLite Query Editor");
  formatter->DetectLanguageByFile(".sql");
  SqliteFile file = app.sqlite.GetSelectedFile();
  LOG() << "Loading last edited query for database" << file.path;
  model->SetPlaceholder("Query results with be displayed here",
                        Theme().kColorSubText);
  IoTask::Run<QList<QString>>(
      this,
      [file] {
        return Database::ExecQueryAndRead<QString>(
            "SELECT editor_query FROM database_file WHERE id=?",
            &Database::ReadStringFromSql, {file.id});
      },
      [this](QList<QString> results) {
        if (!results.isEmpty()) {
          this->query = results.first();
          emit queryChanged();
        }
      });
}

void SqliteQueryEditorController::SaveQuery(const QString &query) {
  SqliteFile file = Application::Get().sqlite.GetSelectedFile();
  LOG() << "Saving editor query of database" << file.path << ':' << query;
  Database::ExecCmdAsync("UPDATE database_file SET editor_query=? WHERE id=?",
                         {query, file.id});
}

QString SqliteQueryEditorController::GetQuery() const { return query; }

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
  QString sql = GetSelectedQuery(text, cursor);
  model->SetPlaceholder("Running query...");
  SqliteSystem::ExecuteQuery(sql).Then(this, [this](SqliteQueryResult result) {
    if (!result.error.isEmpty()) {
      model->SetPlaceholder(result.error, "red");
    } else if (result.is_select) {
      model->SetTable(result.columns, result.rows);
      model->SetPlaceholder();
    } else {
      model->SetPlaceholder("Query executed. Rows affected: " +
                            QString::number(result.rows_affected));
    }
  });
}
