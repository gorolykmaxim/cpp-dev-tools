#include "editor_system.h"

#include "database.h"

static QString ReadOpenCommandFromSql(QSqlQuery& query) {
  return query.value(0).toString();
}

void EditorSystem::Initialize() {
  QList<QString> results = Database::ExecQueryAndReadSync<QString>(
      "SELECT open_command FROM editor", &ReadOpenCommandFromSql);
  open_command = results.first();
}

void EditorSystem::SetOpenCommand(const QString& cmd) {
  open_command = cmd;
  if (!open_command.contains("{}")) {
    open_command += "{}";
  }
  Database::ExecCmdAsync("UPDATE editor SET open_command=?", {open_command});
}

QString EditorSystem::GetOpenCommand() const { return open_command; }
