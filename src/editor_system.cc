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

void EditorSystem::OpenFile(const QString& file, int line, int col) {
  QString link = file;
  if (line > 0) {
    link += ':' + QString::number(line);
  }
  if (col > 0) {
    link += ':' + QString::number(col);
  }
  LOG() << "Opening file link" << link;
  QString cmd = open_command;
  cmd.replace("{}", link);
  cmd.replace("{file}", file);
  OsCommand::RunCmd(cmd, "Code Editor: Failed to open file: " + link);
}
