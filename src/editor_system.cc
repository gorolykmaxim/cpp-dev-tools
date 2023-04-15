#include "editor_system.h"

#include <QProcess>

#include "database.h"

#define LOG() qDebug() << "[EditorSystem]"

static void RunOsCommand(const QString& command) {
  QSharedPointer<QString> output = QSharedPointer<QString>::create();
  QProcess* process = new QProcess();
  QObject::connect(
      process, &QProcess::readyReadStandardOutput, process,
      [process, output] { *output += process->readAllStandardOutput(); });
  QObject::connect(
      process, &QProcess::readyReadStandardError, process,
      [process, output] { *output += process->readAllStandardError(); });
  QObject::connect(process, &QProcess::errorOccurred, process,
                   [process, output, command](QProcess::ProcessError error) {
                     if (error != QProcess::FailedToStart) {
                       return;
                     }
                     *output += "Failed to execute: " + command;
                     emit process->finished(-1);
                   });
  QObject::connect(process, &QProcess::finished, process,
                   [process, output, command] {
                     LOG() << command << "finished:" << *output;
                     process->deleteLater();
                   });
  process->startCommand(command);
}

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

void EditorSystem::OpenFile(const QString& file) {
  LOG() << "Opening file link" << file;
  QString cmd = open_command;
  cmd.replace("{}", file);
  RunOsCommand(cmd);
}
