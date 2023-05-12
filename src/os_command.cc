#include "os_command.h"

#include <QDir>
#include <QProcess>
#include <QSharedPointer>

#include "application.h"

#define LOG() qDebug() << "[OsCommand]"

OsProcess::OsProcess(const QString &command, const QStringList &args)
    : QObject(nullptr),
      command(command),
      args(args),
      exit_code(-1),
      process(this) {}

void OsProcess::Run() {
  QObject::connect(&process, &QProcess::readyReadStandardOutput, this,
                   [this] { output += process.readAllStandardOutput(); });
  QObject::connect(&process, &QProcess::readyReadStandardError, this,
                   [this] { output += process.readAllStandardError(); });
  QObject::connect(&process, &QProcess::errorOccurred, this,
                   [this](QProcess::ProcessError error) {
                     if (error != QProcess::FailedToStart) {
                       return;
                     }
                     output += "Failed to execute: " + command;
                     emit process.finished(-1);
                   });
  QObject::connect(&process, &QProcess::finished, this, [this](int exit_code) {
    output.remove('\r');
    this->exit_code = exit_code;
    emit finished();
    Notification notification(exit_code != 0 ? error_title : success_title);
    notification.is_error = exit_code != 0;
    notification.description = output;
    if (!notification.title.isEmpty()) {
      Application::Get().notification.Post(notification);
    }
    deleteLater();
  });
  if (args.isEmpty()) {
    process.startCommand(command);
  } else {
    process.start(command, args);
  }
}

void OsProcess::WriteToStdin(const QByteArray &data) { process.write(data); }

void OsProcess::FinishWriting() { process.closeWriteChannel(); }

class OpenTerminalInCurrentDirWin : public QObject {
 public:
  void Run() {
    process = new OsProcess("wt", {"/d", QDir::currentPath()});
    QObject::connect(process, &OsProcess::finished, this,
                     &OpenTerminalInCurrentDirWin::GetGitPath);
    process->Run();
  }

 private:
  void GetGitPath() {
    if (process->exit_code == 0) {
      deleteLater();
      return;
    }
    AppendTerminalOpeningError("Microsoft Terminal");
    process = new OsProcess("where git");
    QObject::connect(process, &OsProcess::finished, this,
                     &OpenTerminalInCurrentDirWin::StartGitBash);
    process->Run();
  }

  void StartGitBash() {
    static const QString kExpectedGitSuffix = "\\cmd\\git.exe\n";
    if (process->exit_code != 0 ||
        !process->output.endsWith(kExpectedGitSuffix)) {
      process->exit_code = 1;
      StartCommandPrompt();
      return;
    }
    QString git_bash = process->output;
    git_bash.replace(kExpectedGitSuffix, "\\git-bash.exe");
    process = new OsProcess(git_bash, {"--no-cd"});
    QObject::connect(process, &OsProcess::finished, this,
                     &OpenTerminalInCurrentDirWin::StartCommandPrompt);
    process->Run();
  }

  void StartCommandPrompt() {
    if (process->exit_code == 0) {
      deleteLater();
      return;
    }
    AppendTerminalOpeningError("git-bash");
    process = new OsProcess("cmd", {"/c", "start"});
    QObject::connect(process, &OsProcess::finished, this,
                     &OpenTerminalInCurrentDirWin::LogFailure);
    process->Run();
  }

  void LogFailure() {
    deleteLater();
    if (process->exit_code == 0) {
      return;
    }
    AppendTerminalOpeningError("Command Prompt");
    Notification notification("Failed to open terminal");
    notification.is_error = true;
    notification.description = opening_failures.join('\n');
    Application::Get().notification.Post(notification);
  }

  void AppendTerminalOpeningError(const QString &terminal) {
    QString error = terminal +
                    ": exit code=" + QString::number(process->exit_code) +
                    " output:\n" + process->output;
    if (!process->output.endsWith('\n')) {
      error += '\n';
    }
    opening_failures.append(error);
  }

  OsProcess *process = nullptr;
  QStringList opening_failures;
};

void OsCommand::Run(const QString &command, const QString &error_title,
                    const QString &successful_title) {
  LOG() << "Executing" << command;
  OsProcess *process = new OsProcess(command);
  process->success_title = successful_title;
  process->error_title = error_title;
  process->Run();
}

void OsCommand::OpenTerminalInCurrentDir() {
  LOG() << "Opening terminal in" << QDir::currentPath();
#if __APPLE__
  OsProcess *process = new OsProcess("osascript", {"-", QDir::currentPath()});
  process->error_title = "Faield to open terminal";
  process->Run();
  QString osascript =
      "on run argv\n"
      "tell app \"Terminal\" to do script \"cd \" & item 1 of argv\n"
      "tell app \"Terminal\" to activate\n"
      "end run";
  process->WriteToStdin(osascript.toUtf8());
  process->FinishWriting();
#else
  OpenTerminalInCurrentDirWin *open = new OpenTerminalInCurrentDirWin();
  open->Run();
#endif
}
