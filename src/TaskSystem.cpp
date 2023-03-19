#include "TaskSystem.hpp"

#include "Application.hpp"
#include "Database.hpp"

#define LOG() qDebug() << "[TaskSystem]"

bool ExecutableTask::IsNull() const { return path.isEmpty(); }

Task::Task(const TaskId& id) : id(id) {}

QDebug operator<<(QDebug debug, const Task& task) {
  QDebugStateSaver saver(debug);
  return debug.nospace() << "Task(id=" << task.id << ')';
}

bool TaskExecution::operator==(const TaskExecution& another) const {
  return id == another.id;
}

bool TaskExecution::operator!=(const TaskExecution& another) const {
  return !(*this == another);
}

static QString GetFileName(const QString& path) {
  int pos = path.lastIndexOf('/');
  if (pos < 0) {
    return path;
  } else if (pos == path.size() - 1) {
    return "";
  } else {
    return path.sliced(pos + 1, path.size() - pos - 1);
  }
}

QString TaskSystem::GetName(const Task& task) {
  if (!task.executable.IsNull()) {
    return GetFileName(task.executable.path);
  } else {
    return task.id;
  }
}

void TaskSystem::ExecuteTask(const Task& task) {
  LOG() << "Executing" << task;
  QUuid exec_id = QUuid::createUuid();
  RunningTaskExecution& running_exec = task_executions[exec_id];
  TaskExecutionOutput& exec_output = running_exec.task_execution_output;
  exec_output.output += task.executable.path + "\n";
  TaskExecution& exec = running_exec.task_execution;
  exec.id = exec_id;
  exec.start_time = QDateTime::currentDateTime();
  exec.task_id = task.id;
  exec.task_name = GetName(task);
  QProcess* process = new QProcess();
  running_exec.process = process;
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
  process->startCommand(task.executable.path);
}

void TaskSystem::KillAllTasks() {
  LOG() << "Killing all tasks";
  for (RunningTaskExecution& exec : task_executions.values()) {
    exec.process->kill();
  }
  task_executions.clear();
}

TaskExecution TaskSystem::ReadExecutionFromSql(QSqlQuery& query) {
  TaskExecution exec;
  exec.id = query.value(0).toUuid();
  exec.start_time = query.value(1).toDateTime();
  exec.task_id = query.value(2).toString();
  exec.task_name = query.value(3).toString();
  exec.exit_code = query.value(4).toInt();
  return exec;
}

TaskExecutionOutput TaskSystem::ReadExecutionOutputFromSql(QSqlQuery& query) {
  TaskExecutionOutput exec_output;
  QString indices = query.value(0).toString();
  for (const QString& i : indices.split(',', Qt::SkipEmptyParts)) {
    exec_output.stderr_line_indices.insert(i.toInt());
  }
  exec_output.output = query.value(1).toString();
  return exec_output;
}

void TaskSystem::AppendToExecutionOutput(QUuid id, const QByteArray& data,
                                         bool is_stderr) {
  if (!task_executions.contains(id)) {
    return;
  }
  TaskExecutionOutput& exec_output = task_executions[id].task_execution_output;
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
  if (task_executions.contains(id)) {
    RunningTaskExecution& running_exec = task_executions[id];
    TaskExecution& exec = running_exec.task_execution;
    if (process->error() == QProcess::FailedToStart ||
        process->exitStatus() == QProcess::CrashExit) {
      exec.exit_code = -1;
    } else {
      exec.exit_code = process->exitCode();
    }
    LOG() << "Task" << id << "finished with code" << *exec.exit_code;
    TaskExecutionOutput& exec_output = running_exec.task_execution_output;
    const Project& project = Application::Get().project.GetCurrentProject();
    QStringList indices;
    for (int i : exec_output.stderr_line_indices) {
      indices.append(QString::number(i));
    }
    Database::ExecCmdAsync(
        "INSERT INTO task_execution VALUES(?,?,?,?,?,?,?,?)",
        {exec.id, project.id, exec.start_time, exec.task_id, exec.task_name,
         *exec.exit_code, indices.join(','), exec_output.output});
    task_executions.remove(id);
    emit executionFinished(id);
  }
  process->deleteLater();
}

void TaskSystem::FetchExecutions(
    QObject* requestor, QUuid project_id,
    const std::function<void(const QList<TaskExecution>&)>& callback) const {
  LOG() << "Fetching executions for project" << project_id;
  QList<TaskExecution> execs;
  for (QUuid id : task_executions.keys()) {
    execs.append(task_executions[id].task_execution);
  }
  std::sort(execs.begin(), execs.end(),
            [](const TaskExecution& a, const TaskExecution& b) {
              return a.start_time < b.start_time;
            });
  Application::Get().RunIOTask<QList<TaskExecution>>(
      requestor,
      [project_id] {
        return Database::ExecQueryAndRead<TaskExecution>(
            "SELECT id, start_time, task_id, task_name, exit_code FROM "
            "task_execution "
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
  if (task_executions.contains(execution_id)) {
    callback(task_executions[execution_id].task_execution_output);
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

void TaskSystem::CancelExecution(QUuid execution_id, bool forcefully) {
  if (!task_executions.contains(execution_id)) {
    return;
  }
  LOG() << "Attempting to cancel execution" << execution_id
        << "forcefully:" << forcefully;
  RunningTaskExecution& exec = task_executions[execution_id];
  if (!exec.process) {
    return;
  }
  if (forcefully) {
    exec.process->kill();
  } else {
    exec.process->terminate();
  }
}
