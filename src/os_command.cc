#include "os_command.h"

#include <QDir>
#include <QProcess>
#include <QSharedPointer>

#include "application.h"

#define LOG() qDebug() << "[OsCommand]"

static void Exec(QProcess *process, const QString &command,
                 const QString &error_title) {
  QSharedPointer<QString> output = QSharedPointer<QString>::create();
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

void OsCommand::Run(const QString &command, const QString &error_title) {
  LOG() << "Executing" << command;
  Exec(new QProcess(), command, error_title);
}

void OsCommand::OpenTerminalInCurrentDir() {
  LOG() << "Opening terminal in" << QDir::currentPath();
  QString osascript =
      "tell app \"Terminal\" to do script \"cd $TARGET_DIR\"\n"
      "tell app \"Terminal\" to activate\n";
  QProcess *process = new QProcess();
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("TARGET_DIR", QDir::currentPath());
  process->setProcessEnvironment(env);
  Exec(process, "osascript -", "Faield to open terminal");
  process->write(osascript.toUtf8());
  process->closeWriteChannel();
}
