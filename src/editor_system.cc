#include "editor_system.h"

#include "database.h"
#include "os_command.h"

#define LOG() qDebug() << "[EditorSystem]"

void EditorSystem::Initialize() {
  LOG() << "Initializing";
  QList<QString> results = Database::ExecQueryAndReadSync<QString>(
      "SELECT open_command FROM editor", &Database::ReadStringFromSql);
  open_command = results.first();
  LOG() << "Command to open editor:" << open_command;
}

void EditorSystem::OpenFile(const QString& file, int column, int row) {
  QString link = file;
  if (column > 0) {
    link += ':' + QString::number(column);
  }
  if (row > 0) {
    link += ':' + QString::number(row);
  }
  LOG() << "Opening file link" << link;
  QString cmd = open_command;
  cmd.replace("{}", link);
  OsCommand::Run(cmd, "Failed to open file in editor: " + link);
}
