#include "TaskExecutor.hpp"

#define LOG() qDebug() << "[TaskExecutor]"

QString TaskExecution::ShortenCommand(QString cmd, const Project& project) {
  if (cmd.startsWith(project.path)) {
    cmd.replace(project.path, ".");
  }
  return cmd;
}

void TaskExecutor::ExecuteTask(const QString& command) {
  LOG() << "Executing task" << command;
  QUuid exec_id = QUuid::createUuid();
  TaskExecution& exec = active_executions[exec_id];
  exec.id = exec_id;
  exec.start_time = QDateTime::currentDateTime();
  exec.command = command;
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
  if (!active_execution_outputs.contains(id)) {
    return;
  }
  active_execution_outputs[id] += data;
  emit executionOutputChanged(id);
}

void TaskExecutor::FinishExecution(QUuid id, QProcess* process) {
  if (active_executions.contains(id)) {
    TaskExecution& exec = active_executions[id];
    if (process->error() == QProcess::FailedToStart ||
        process->exitStatus() == QProcess::CrashExit) {
      exec.exit_code = -1;
    } else {
      exec.exit_code = process->exitCode();
    }
    LOG() << "Task" << id << "finished with code" << *exec.exit_code;
    emit executionFinished(id);
  }
  process->deleteLater();
}

void TaskExecutor::FetchExecutions(
    QObject*, QUuid project_id,
    const std::function<void(const QList<TaskExecution>&)>& callback) const {
  LOG() << "Fetching executions for project" << project_id;
  QList<TaskExecution> execs;
  for (QUuid id : active_executions.keys()) {
    execs.append(active_executions[id]);
  }
  std::sort(execs.begin(), execs.end(),
            [](const TaskExecution& a, const TaskExecution& b) {
              return a.start_time < b.start_time;
            });
  callback(execs);
}

void TaskExecutor::FetchExecutionOutput(
    QObject*, QUuid execution_id,
    const std::function<void(const QString&)>& callback) const {
  if (!active_execution_outputs.contains(execution_id)) {
    return;
  }
  LOG() << "Fetching output of execution" << execution_id;
  callback(active_execution_outputs[execution_id]);
}
