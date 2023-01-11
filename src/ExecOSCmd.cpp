#include "ExecOSCmd.hpp"
#include "Process.hpp"

#define LOG() qDebug() << "[ExecOSCmd]"

ExecOSCmd::ExecOSCmd(const QString& cmd, QPtr<QProcess> proc)
    : cmd(cmd), proc(proc ? proc : QPtr<QProcess>::create()) {
  EXEC_NEXT(Run);
  on_output = [] (const QString&) {};
}

void ExecOSCmd::Run(AppData& app) {
  QPtr<ExecOSCmd> self = ProcessSharedPtr(app, this);
  LOG() << "Executing OS command:" << cmd;
  QObject::connect(proc.get(), &QProcess::readyReadStandardOutput, proc.get(), [self] () {
    QString data(self->proc->readAllStandardOutput());
    self->on_output(data);
  }, Qt::QueuedConnection);
  QObject::connect(proc.get(), &QProcess::readyReadStandardError, proc.get(), [self] () {
    QString data(self->proc->readAllStandardError());
    self->on_output(data);
  }, Qt::QueuedConnection);
  QObject::connect(proc.get(), &QProcess::errorOccurred, proc.get(), [&app, self] (QProcess::ProcessError error) {
    LOG() << "Failed to execute" << self->cmd << error;
    WakeUpAndExecuteProcess(app, *self);
  }, Qt::QueuedConnection);
  QObject::connect(proc.get(), &QProcess::finished, proc.get(), [&app, self] (int code, QProcess::ExitStatus) {
    LOG() << self->cmd << "finished with exit code" << code;
    WakeUpAndExecuteProcess(app, *self);
  }, Qt::QueuedConnection);
  proc->startCommand(cmd);
  EXEC_NEXT(Noop);
}
