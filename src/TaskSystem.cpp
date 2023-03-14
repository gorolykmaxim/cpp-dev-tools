#include "TaskSystem.hpp"

#define LOG() qDebug() << "[TaskSystem]"

QString TaskExecution::ShortenCommand(QString cmd, const Project& project) {
  if (cmd.startsWith(project.path)) {
    cmd.replace(project.path, ".");
  }
  return cmd;
}

void TaskSystem::ExecuteTask(const QString& command) {
  LOG() << "Executing task" << command;
  QUuid exec_id = QUuid::createUuid();
  active_execution_outputs[exec_id] = TaskExecutionOutput();
  TaskExecution& exec = active_executions[exec_id];
  exec.id = exec_id;
  exec.start_time = QDateTime::currentDateTime();
  exec.command = command;
  QProcess* process = new QProcess();
  active_processes[exec_id] = process;
  QObject::connect(process, &QProcess::readyReadStandardOutput, this,
                   [this, process, exec_id] {
                     AppendToExecutionOutput(
                         exec_id, process->readAllStandardOutput(), false);
                   });
  QObject::connect(process, &QProcess::readyReadStandardError, this,
                   [this, process, exec_id] {
                     AppendToExecutionOutput(
                         exec_id, process->readAllStandardError(), true);
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

void TaskSystem::KillAllTasks() {
  LOG() << "Killing all tasks";
  for (QProcess* process : active_processes.values()) {
    process->kill();
  }
  active_executions.clear();
  active_execution_outputs.clear();
  active_processes.clear();
}

void TaskSystem::AppendToExecutionOutput(QUuid id, const QByteArray& data,
                                         bool is_stderr) {
  if (!active_execution_outputs.contains(id)) {
    return;
  }
  TaskExecutionOutput& exec_output = active_execution_outputs[id];
  QString data_str(data);
  if (is_stderr) {
    int lines_before = exec_output.output.count('\n');
    for (int i = 0; i < data_str.count('\n'); i++) {
      exec_output.stderr_line_indices.insert(i + lines_before);
    }
  }
  exec_output.output += data_str;
  emit executionOutputChanged(id);
}

void TaskSystem::FinishExecution(QUuid id, QProcess* process) {
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
  active_processes.remove(id);
}

void TaskSystem::FetchExecutions(
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

void TaskSystem::FetchExecutionOutput(
    QObject*, QUuid execution_id,
    const std::function<void(const TaskExecutionOutput&)>& callback) const {
  if (!active_execution_outputs.contains(execution_id)) {
    return;
  }
  LOG() << "Fetching output of execution" << execution_id;
  callback(active_execution_outputs[execution_id]);
}
