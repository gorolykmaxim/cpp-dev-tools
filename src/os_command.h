#ifndef OSCOMMAND_H
#define OSCOMMAND_H

#include <QProcess>
#include <QString>

#include "promise.h"

struct OsProcess {
  int exit_code = -1;
  QString output;
};

class OsCommand {
 public:
  static Promise<OsProcess> RunCmd(const QString& cmd,
                                   const QString& error_title = "",
                                   const QString& success_title = "");
  static Promise<OsProcess> Run(const QString& cmd,
                                const QStringList& args = {},
                                const QString& input = "",
                                const QString& error_title = "",
                                const QString& success_title = "");
  static void InitTerminals();
  static void OpenTerminalInCurrentDir();
};

#endif  // OSCOMMAND_H
