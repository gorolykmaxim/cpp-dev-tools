#include "os_command.h"

#include <QDir>
#include <QProcess>
#include <QSharedPointer>

#include "application.h"
#include "database.h"
#include "io_task.h"

#define LOG() qDebug() << "[OsCommand]"

static const QString kMacOsTerminalName = "Terminal";
static const QString kMicrosoftTerminalName = "Microsoft Terminal";
static const QString kGitBashName = "Git Bash";
static const QString kCommandPromptName = "Command Prompt";

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
                                  const QString &success_title,
                                  int expected_exit_code) {
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
                                     ' ' + proc->arguments().join(' ');
                     emit proc->finished(-1);
                   });
  QObject::connect(proc, &QProcess::finished, proc,
                   [promise, data, proc, error_title, success_title,
                    expected_exit_code](int exit_code, QProcess::ExitStatus) {
                     data->exit_code = exit_code;
                     data->output.remove('\r');
                     Notification notification(exit_code != expected_exit_code
                                                   ? error_title
                                                   : success_title);
                     notification.is_error = exit_code != expected_exit_code;
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

void OsCommand::InitTerminals() {
  QStringList default_terminal_order;
#if __APPLE__
  default_terminal_order = {kMacOsTerminalName};
#else
  default_terminal_order = {kMicrosoftTerminalName, kGitBashName,
                            kCommandPromptName};
#endif
  QList<Database::Cmd> cmds;
  for (int i = 0; i < default_terminal_order.size(); i++) {
    cmds.append(Database::Cmd("INSERT OR IGNORE INTO terminal VALUES(?, ?)",
                              {default_terminal_order[i], i}));
  }
  Database::ExecCmdsAsync(cmds);
}

static Promise<OsProcess> OpenMacOsTerminal() {
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
  return OsCommand::Run("osascript", {"-", QDir::currentPath()}, osascript);
}

static Promise<OsProcess> OpenMicrosoftTerminal() {
  return OsCommand::Run("wt", {"/d", QDir::currentPath()});
}

static Promise<OsProcess> OpenGitBash() {
  return OsCommand::Run("where", {"git"})
      .Then<OsProcess>(&Application::Get().gui_app, [](OsProcess proc) {
        static const QString kExpectedGitSuffix = "\\cmd\\git.exe\n";
        if (proc.exit_code != 0 || !proc.output.endsWith(kExpectedGitSuffix)) {
          proc.exit_code = 1;
          return Promise<OsProcess>(proc);
        }
        QString git_bash = proc.output;
        git_bash.replace(kExpectedGitSuffix, "\\git-bash.exe");
        return OsCommand::Run(git_bash, {"--no-cd"});
      });
}

static Promise<OsProcess> OpenCommandPrompt() {
  return OsCommand::Run("cmd", {"/c", "start"});
}

static void TryOpenNextTerminal(QStringList terminals,
                                QSharedPointer<QStringList> failures) {
  static const QHash<QString, std::function<Promise<OsProcess>()>>
      kTerminalNameToFunc = {
          {kMacOsTerminalName, OpenMacOsTerminal},
          {kMicrosoftTerminalName, OpenMicrosoftTerminal},
          {kGitBashName, OpenGitBash},
          {kCommandPromptName, OpenCommandPrompt},
      };
  if (terminals.isEmpty()) {
    Notification notification("Failed to open terminal");
    notification.is_error = true;
    notification.description = failures->join('\n');
    Application::Get().notification.Post(notification);
    return;
  }
  QString terminal = terminals.takeFirst();
  std::function<Promise<OsProcess>()> f = kTerminalNameToFunc[terminal];
  f().Then(&Application::Get().gui_app, [terminals = std::move(terminals),
                                         failures, terminal](OsProcess p) {
    if (p.exit_code == 0) {
      return;
    }
    QString error = terminal + ": exit code=" + QString::number(p.exit_code) +
                    " output:\n" + p.output;
    if (!p.output.endsWith('\n')) {
      error += '\n';
    }
    failures->append(error);
    TryOpenNextTerminal(terminals, failures);
  });
}

void OsCommand::OpenTerminalInCurrentDir() {
  LOG() << "Opening terminal in" << QDir::currentPath();
  IoTask::Run<QStringList>(
      &Application::Get().gui_app,
      [] {
        return Database::ExecQueryAndRead<QString>(
            "SELECT name FROM terminal ORDER BY priority",
            &Database::ReadStringFromSql);
      },
      [](QStringList terminals) {
        auto failures = QSharedPointer<QStringList>::create();
        TryOpenNextTerminal(terminals, failures);
      });
}

void OsCommand::LoadEnvironmentVariablesOnMac(QObject *ctx) {
  Run("zsh", {"-c", "env"})
      .Then<OsProcess>(
          ctx,
          [](OsProcess p) {
            if (p.exit_code != 0) {
              LOG() << "Failed to read environment variables via zsh:"
                    << p.output;
              return Run("bash", {"-c", "env"});
            } else {
              return Promise<OsProcess>(p);
            }
          })
      .Then(ctx, [](OsProcess p) {
        if (p.exit_code != 0) {
          LOG() << "Failed to read environment variables via bash:" << p.output;
          return;
        }
        for (const QString &line : p.output.split('\n', Qt::SkipEmptyParts)) {
          int i = line.indexOf('=');
          if (i < 0) {
            continue;
          }
          QString name = line.sliced(0, i);
          QString value = line.sliced(i + 1);
          qputenv(name.toStdString().c_str(), value.toUtf8());
        }
      });
}
