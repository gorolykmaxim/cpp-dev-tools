#include "ExecOSCmd.hpp"
#include "Process.hpp"

ExecOSCmd::ExecOSCmd(const QString& cmd, QPtr<QProcess> proc)
    : cmd(cmd), proc(proc ? proc : QPtr<QProcess>::create()) {
  EXEC_NEXT(Run);
  on_output = [] (const QString&) {};
}

void ExecOSCmd::Run(AppData& app) {
  QPtr<ExecOSCmd> self = ProcessSharedPtr(app, this);
  qDebug() << "Executing OS command:" << cmd;
  QObject::connect(proc.get(), &QProcess::readyReadStandardOutput, [self] () {
    QString data(self->proc->readAllStandardOutput());
    self->on_output(data);
  });
  QObject::connect(proc.get(), &QProcess::readyReadStandardError, [self] () {
    QString data(self->proc->readAllStandardError());
    self->on_output(data);
  });
  QObject::connect(proc.get(), &QProcess::errorOccurred, [&app, self] (QProcess::ProcessError error) {
    qDebug() << "Failed to execute" << self->cmd << error;
    WakeUpAndExecuteProcess(app, *self);
  });
  QObject::connect(proc.get(), &QProcess::finished, [&app, self] (int code, QProcess::ExitStatus) {
    qDebug() << self->cmd << "finished with exit code" << code;
    WakeUpAndExecuteProcess(app, *self);
  });
  proc->startCommand(cmd);
  EXEC_NEXT(Noop);
}
