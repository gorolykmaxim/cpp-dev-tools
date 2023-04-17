#include "editor_system.h"

#include <QProcess>

#include "database.h"

#define LOG() qDebug() << "[EditorSystem]"

static void RunOsCommand(const QString& command, const QString& error_title) {
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
                   [process, output, command, error_title](int exit_code) {
                     if (exit_code != 0) {
                       Notification notification;
                       notification.is_error = true;
                       notification.title = error_title;
                       notification.description = *output;
                       Application::Get().notification.Post(notification);
                     }
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

void EditorSystem::OpenFile(const QString& file) {
  LOG() << "Opening file link" << file;
  QString cmd = open_command;
  cmd.replace("{}", file);
  RunOsCommand(cmd, "Failed to open file in editor: " + file);
}
