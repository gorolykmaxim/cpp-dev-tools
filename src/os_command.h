#ifndef OSCOMMAND_H
#define OSCOMMAND_H

#include <QProcess>
#include <QString>

class OsCommand {
 public:
  static void Run(const QString& command, const QString& error_title);
  static void OpenTerminalInCurrentDir();
};

#endif  // OSCOMMAND_H
