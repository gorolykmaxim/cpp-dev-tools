#include "TaskExecutor.hpp"

#include <QProcess>
#include <QSharedPointer>

#define LOG() qDebug() << "[TaskExecutor]"

void TaskExecutor::ExecuteTask(const QString& command) {
  LOG() << "Executing task" << command;
  TaskExecution exec;
  QUuid exec_id = exec.id = QUuid::createUuid();
  exec.start_time = QDateTime::currentDateTime();
  exec.command = command;
  executions.append(exec);
  QSharedPointer<QProcess> process = QSharedPointer<QProcess>::create();
  QObject::connect(process.get(), &QProcess::readyReadStandardOutput, this,
                   [this, process, exec_id] {
                     AppendToExecutionOutput(exec_id,
                                             process->readAllStandardOutput());
                   });
  QObject::connect(process.get(), &QProcess::readyReadStandardError, this,
                   [this, process, exec_id] {
                     AppendToExecutionOutput(exec_id,
                                             process->readAllStandardError());
                   });
  QObject::connect(process.get(), &QProcess::errorOccurred, this,
                   [this, process, exec_id](QProcess::ProcessError) {
                     SetExecutionExitCode(exec_id, -1);
                   });
  QObject::connect(process.get(), &QProcess::finished, this,
                   [this, process, exec_id](int code, QProcess::ExitStatus) {
                     SetExecutionExitCode(exec_id, code);
                   });
  process->startCommand(command);
}

void TaskExecutor::AppendToExecutionOutput(QUuid id, const QByteArray& data) {
  if (TaskExecution* exec = FindExecutionById(id)) {
    exec->output += data;
    emit executionOutputChanged(id);
  }
}

void TaskExecutor::SetExecutionExitCode(QUuid id, int code) {
  if (TaskExecution* exec = FindExecutionById(id)) {
    LOG() << "Task" << id << "finished with code" << code;
    exec->exit_code = code;
    emit executionFinished(id);
  }
}

TaskExecution* TaskExecutor::FindExecutionById(QUuid id) {
  for (TaskExecution& exec : executions) {
    if (exec.id == id) {
      return &exec;
    }
  }
  return nullptr;
}
