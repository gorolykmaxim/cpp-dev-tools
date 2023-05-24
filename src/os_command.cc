#include "os_command.h"

#include <QDir>
#include <QProcess>
#include <QSharedPointer>

#include "application.h"

#define LOG() qDebug() << "[OsCommand]"

Promise<OsProcess> OsCommand::RunCmd(const QString &cmd,
                                     const QString &error_title,
                                     const QString &success_title) {
  QStringList args = QProcess::splitCommand(cmd);
  QString program = args.takeFirst();
  return Run(program, args, "", error_title, success_title);
}

Promise<OsProcess> OsCommand::Run(const QString &cmd, const QStringList &args,
                                  const QString &input,
                                  const QString &error_title,
                                  const QString &success_title) {
  LOG() << "Executing" << cmd << args.join(' ');
  auto promise = QSharedPointer<QPromise<OsProcess>>::create();
  auto data = QSharedPointer<OsProcess>::create();
  auto proc = new QProcess();
  QObject::connect(proc, &QProcess::readyReadStandardError, proc, [data, proc] {
    data->output += proc->readAllStandardError();
  });
  QObject::connect(
      proc, &QProcess::readyReadStandardOutput, proc,
      [data, proc] { data->output += proc->readAllStandardOutput(); });
  QObject::connect(proc, &QProcess::errorOccurred, proc,
                   [data, proc](QProcess::ProcessError error) {
                     if (error != QProcess::FailedToStart) {
                       return;
                     }
                     data->output += "Failed to execute: " + proc->program() +
                                     proc->arguments().join(' ');
                     emit proc->finished(-1);
                   });
  QObject::connect(
      proc, &QProcess::finished, proc,
      [promise, data, proc, error_title, success_title](int exit_code,
                                                        QProcess::ExitStatus) {
        data->exit_code = exit_code;
        data->output.remove('\r');
        Notification notification(exit_code != 0 ? error_title : success_title);
        notification.is_error = exit_code != 0;
        notification.description = data->output;
        if (!notification.title.isEmpty()) {
          Application::Get().notification.Post(notification);
        }
        promise->addResult(*data);
        promise->finish();
        proc->deleteLater();
      });
  proc->start(cmd, args);
  if (!input.isEmpty()) {
    proc->write(input.toUtf8());
    proc->closeWriteChannel();
  }
  return promise->future();
}

void OsCommand::OpenTerminalInCurrentDir() {
  LOG() << "Opening terminal in" << QDir::currentPath();
#if __APPLE__
  QString osascript =
      "on run argv\n"
      " set cmd to \"cd \" & item 1 of argv\n"
      " tell app \"Terminal\"\n"
      "     activate\n"
      "     if (count of windows) = 0 then\n"
      "         do script cmd\n"
      "     else\n"
      "         do script cmd in selected tab of front window\n"
      "     end if\n"
      " end tell\n"
      "end run";
  Run("osascript", {"-", QDir::currentPath()}, osascript,
      "Faield to open terminal");
#else
  auto failures = QSharedPointer<QStringList>::create();
  auto LogError = [failures](const QString &terminal, const OsProcess &proc) {
    QString error = terminal +
                    ": exit code=" + QString::number(proc.exit_code) +
                    " output:\n" + proc.output;
    if (!proc.output.endsWith('\n')) {
      error += '\n';
    }
    failures->append(error);
  };
  QObject *ctx = &Application::Get().gui_app;
  Run("wt", {"/d", QDir::currentPath()})
      .Then<OsProcess>(
          ctx,
          [LogError, ctx](OsProcess proc) {
            if (proc.exit_code == 0) {
              return Promise<OsProcess>();
            }
            LogError("Microsoft Terminal", proc);
            return Run("where", {"git"})
                .Then<OsProcess>(ctx, [](OsProcess proc) {
                  static const QString kExpectedGitSuffix = "\\cmd\\git.exe\n";
                  if (proc.exit_code != 0 ||
                      !proc.output.endsWith(kExpectedGitSuffix)) {
                    proc.exit_code = 1;
                    return Promise<OsProcess>(proc);
                  }
                  QString git_bash = proc.output;
                  git_bash.replace(kExpectedGitSuffix, "\\git-bash.exe");
                  return Run(git_bash, {"--no-cd"});
                });
          })
      .Then<OsProcess>(ctx,
                       [LogError](OsProcess proc) {
                         if (proc.exit_code == 0) {
                           return Promise<OsProcess>();
                         }
                         LogError("git-bash", proc);
                         return Run("cmd", {"/c", "start"});
                       })
      .Then(ctx, [LogError, failures](OsProcess proc) {
        if (proc.exit_code == 0) {
          return;
        }
        LogError("Command Prompt", proc);
        Notification notification("Failed to open terminal");
        notification.is_error = true;
        notification.description = failures->join('\n');
        Application::Get().notification.Post(notification);
      });
#endif
}
