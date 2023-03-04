#include "TaskExecutor.hpp"

#define LOG() qDebug() << "[TaskExecutor]"

void TaskExecutor::ExecuteTask(const QString& command) {
  LOG() << "Executing task" << command;
  TaskExecution exec;
  QUuid exec_id = exec.id = QUuid::createUuid();
  exec.start_time = QDateTime::currentDateTime();
  exec.command = command;
  executions.append(exec);
  QProcess* process = new QProcess();
  QObject::connect(process, &QProcess::readyReadStandardOutput, this,
                   [this, process, exec_id] {
                     AppendToExecutionOutput(exec_id,
                                             process->readAllStandardOutput());
                   });
  QObject::connect(process, &QProcess::readyReadStandardError, this,
                   [this, process, exec_id] {
                     AppendToExecutionOutput(exec_id,
                                             process->readAllStandardError());
                   });
  QObject::connect(process, &QProcess::errorOccurred, this,
                   [this, process, exec_id](QProcess::ProcessError) {
                     FinishExecution(exec_id, process);
                   });
  QObject::connect(process, &QProcess::finished, this,
                   [this, process, exec_id](int, QProcess::ExitStatus) {
                     FinishExecution(exec_id, process);
                   });
  process->startCommand(command);
}

void TaskExecutor::AppendToExecutionOutput(QUuid id, const QByteArray& data) {
  if (TaskExecution* exec = FindExecutionById(id)) {
    exec->output += data;
    emit executionOutputChanged(id);
  }
}

void TaskExecutor::FinishExecution(QUuid id, QProcess* process) {
  if (TaskExecution* exec = FindExecutionById(id)) {
    if (process->error() == QProcess::FailedToStart ||
        process->exitStatus() == QProcess::CrashExit) {
      exec->exit_code = -1;
    } else {
      exec->exit_code = process->exitCode();
    }
    LOG() << "Task" << id << "finished with code" << *exec->exit_code;
    emit executionFinished(id);
  }
  process->deleteLater();
}

TaskExecution* TaskExecutor::FindExecutionById(QUuid id) {
  for (TaskExecution& exec : executions) {
    if (exec.id == id) {
      return &exec;
    }
  }
  return nullptr;
}
