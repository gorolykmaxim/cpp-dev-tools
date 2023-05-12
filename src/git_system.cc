#include "git_system.h"

#include <QDir>
#include <QProcess>
#include <QTextStream>

#include "application.h"
#include "os_command.h"

#define LOG() qDebug() << "[GitSystem]"

QList<QString> GitSystem::FindIgnoredPathsSync() {
  LOG() << "Looking for ignored files";
  QList<QString> results;
  QProcess proc;
  proc.start("git", QStringList() << "status"
                                  << "--porcelain=v1"
                                  << "--ignored");
  bool failed_to_start = !proc.waitForFinished();
  if (failed_to_start || proc.exitCode() != 0) {
    Notification notification("Git: Failed to find ignored files");
    notification.is_error = true;
    if (failed_to_start) {
      notification.description = "Failed to execute: " + proc.program() + ' ' +
                                 proc.arguments().join(' ');
    } else {
      notification.description = proc.readAllStandardError();
    }
    Application::Get().notification.Post(notification);
  } else {
    QTextStream stream(&proc);
    QDir folder = QDir::current();
    while (!stream.atEnd()) {
      QString line = stream.readLine().trimmed();
      QStringList row = line.split(' ', Qt::SkipEmptyParts);
      if (row.size() < 2 || row[0] != "!!") {
        continue;
      }
      results.append(folder.filePath(row[1]));
    }
  }
  return results;
}

void GitSystem::Pull() {
  Application::Get().notification.Post(Notification("Git: Pulling changes..."));
  OsCommand::Run("git pull", "Git: Failed to pull changes",
                 "Git: Changes pulled");
}

void GitSystem::Push() {
  Application::Get().notification.Post(Notification("Git: Pushing changes..."));
  OsCommand::Run("git push", "Git: Failed to push changes", "");
}
