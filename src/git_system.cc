#include "git_system.h"

#include <QDir>
#include <QProcess>
#include <QTextStream>

#include "application.h"

#define LOG() qDebug() << "[GitSystem]"

QList<QString> GitSystem::FindIgnoredPathsSync() {
  LOG() << "Looking for ignored files";
  QList<QString> results;
  QProcess proc;
  proc.start("git", QStringList() << "status"
                                  << "--porcelain=v1"
                                  << "--ignored");
  if (!proc.waitForFinished()) {
    return results;
  }
  if (proc.exitCode() != 0) {
    Notification notification;
    notification.is_error = true;
    notification.title = "Failed to find files ignored by git";
    notification.description = proc.readAllStandardError();
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
