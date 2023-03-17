#include "TaskSystem.hpp"

#include "Application.hpp"
#include "Database.hpp"

#define LOG() qDebug() << "[TaskSystem]"

bool TaskExecution::operator==(const TaskExecution& another) const {
  return id == another.id;
}

bool TaskExecution::operator!=(const TaskExecution& another) const {
  return !(*this == another);
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

TaskExecution TaskSystem::ReadExecutionFromSql(QSqlQuery& query) {
  TaskExecution exec;
  exec.id = query.value(0).toUuid();
  exec.start_time = query.value(1).toDateTime();
  exec.command = query.value(2).toString();
  exec.exit_code = query.value(3).toInt();
  return exec;
}

TaskExecutionOutput TaskSystem::ReadExecutionOutputFromSql(QSqlQuery& query) {
  TaskExecutionOutput exec_output;
  QJsonDocument doc = QJsonDocument::fromJson(query.value(0).toByteArray());
  QJsonArray indices = doc.array();
  for (int i = 0; i < indices.size(); i++) {
    exec_output.stderr_line_indices.insert(indices[i].toInt());
  }
  exec_output.output = query.value(1).toString();
  return exec_output;
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
    TaskExecutionOutput& exec_output = active_execution_outputs[id];
    const Project& project = Application::Get().project.GetCurrentProject();
    QJsonArray indices;
    for (int i : exec_output.stderr_line_indices) {
      indices.append(i);
    }
    Database::ExecCmdAsync(
        "INSERT INTO task_execution VALUES(?,?,?,?,?,?,?,?)",
        {exec.id, exec.id, project.id, exec.start_time, exec.command,
         *exec.exit_code, QJsonDocument(indices).toJson(QJsonDocument::Compact),
         exec_output.output});
    active_executions.remove(id);
    active_execution_outputs.remove(id);
    active_processes.remove(id);
    emit executionFinished(id);
  }
  process->deleteLater();
}

void TaskSystem::FetchExecutions(
    QObject* requestor, QUuid project_id,
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
  Application::Get().RunIOTask<QList<TaskExecution>>(
      requestor,
      [project_id] {
        return Database::ExecQueryAndRead<TaskExecution>(
            "SELECT id, start_time, command, exit_code FROM task_execution "
            "WHERE project_id=? ORDER BY start_time",
            &TaskSystem::ReadExecutionFromSql, {project_id});
      },
      [execs, callback](QList<TaskExecution> result) {
        for (const TaskExecution& exec : execs) {
          // Due to possible races between threads and callback scheduling
          // on the main thread, some of "execs" might have already been
          // inserted into the database and might be included in "result".
          if (!result.contains(exec)) {
            result.append(exec);
          }
        }
        callback(result);
      });
}

void TaskSystem::FetchExecutionOutput(
    QObject* requestor, QUuid execution_id,
    const std::function<void(const TaskExecutionOutput&)>& callback) const {
  LOG() << "Fetching output of execution" << execution_id;
  if (active_execution_outputs.contains(execution_id)) {
    callback(active_execution_outputs[execution_id]);
    return;
  }
  Application::Get().RunIOTask<QList<TaskExecutionOutput>>(
      requestor,
      [execution_id] {
        return Database::ExecQueryAndRead<TaskExecutionOutput>(
            "SELECT stderr_line_indices, output FROM task_execution WHERE id=?",
            &TaskSystem::ReadExecutionOutputFromSql, {execution_id});
      },
      [callback](QList<TaskExecutionOutput> result) {
        callback(result.isEmpty() ? TaskExecutionOutput() : result[0]);
      });
}
