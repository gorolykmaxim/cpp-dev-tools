#ifndef OSCOMMAND_H
#define OSCOMMAND_H

#include <QProcess>
#include <QString>

class OsProcess : public QObject {
  Q_OBJECT
 public:
  explicit OsProcess(const QString& command, const QStringList& args = {});
  void Run();
  void WriteToStdin(const QByteArray& data);
  void FinishWriting();

  QString command;
  QStringList args;
  QString output;
  QString error_title;
  int exit_code;
 signals:
  void finished();

 private:
  QProcess process;
};

class OsCommand {
 public:
  static void Run(const QString& command, const QString& error_title);
  static void OpenTerminalInCurrentDir();
};

#endif  // OSCOMMAND_H
