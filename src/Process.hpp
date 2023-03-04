#pragma once

#include <QDebug>
#include <QProcess>

class Process : public QObject {
  Q_OBJECT
 public:
  Process(const QString& command, QObject* parent = nullptr);
  void Start();
  const QString& GetOutput() const;
  const QString& GetCommand() const;
  int GetExitCode() const;

 signals:
  void outputChanged();
  void finished();

 private:
  QString command;
  QString output;
  QProcess process;
};
