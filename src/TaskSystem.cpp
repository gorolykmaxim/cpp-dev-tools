#include "TaskSystem.hpp"

#include "Application.hpp"
#include "Database.hpp"

#define LOG() qDebug() << "[TaskSystem]"

template <typename T>
using Ptr = QSharedPointer<T>;

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

class RunExecutableCommand : public RunTaskCommand {
 public:
  RunExecutableCommand(const Task& task) : executable(task.executable) {
    QObject::connect(
        &process, &QProcess::readyReadStandardOutput, this,
        [this] { emit outputWritten(process.readAllStandardOutput(), false); });
    QObject::connect(&process, &QProcess::readyReadStandardError, this, [this] {
      emit outputWritten(process.readAllStandardError(), true);
    });
    QObject::connect(&process, &QProcess::errorOccurred, this,
                     [this](QProcess::ProcessError error) {
                       if (error != QProcess::FailedToStart) {
                         return;
                       }
                       emit outputWritten("Failed to start executable\n", true);
                       emit finished(-1);
                     });
    QObject::connect(&process, &QProcess::finished, this,
                     [this](int exit_code, QProcess::ExitStatus) {
                       emit finished(exit_code);
                     });
  }

  virtual void Start() override {
    process.startCommand(executable.path);
    emit outputWritten(executable.path + '\n', false);
  }

  virtual void Cancel(bool forcefully) override {
    if (forcefully) {
      process.kill();
    } else {
      process.terminate();
    }
  }

 private:
  QProcess process;
  ExecutableTask executable;
};

class RunUntilFailCommand : public RunTaskCommand {
 public:
  RunUntilFailCommand(Ptr<RunTaskCommand> target, QUuid execution_id,
                      QHash<QUuid, TaskExecutionOutput>& outputs)
      : target(target), execution_id(execution_id), outputs(outputs) {
    QObject::connect(target.get(), &RunTaskCommand::outputWritten, this,
                     [this](const QString& output, bool is_stderr) {
                       emit outputWritten(output, is_stderr);
                     });
    QObject::connect(target.get(), &RunTaskCommand::finished, this,
                     &RunUntilFailCommand::HandleTargetFinished);
  }

  virtual void Start() override { target->Start(); }

  virtual void Cancel(bool forcefully) override { target->Cancel(forcefully); }

 private:
  void HandleTargetFinished(int exit_code) {
    if (exit_code != 0) {
      emit finished(exit_code);
    } else {
      outputs[execution_id] = TaskExecutionOutput();
      target->Start();
    }
  }

  Ptr<RunTaskCommand> target;
  QUuid execution_id;
  QHash<QUuid, TaskExecutionOutput>& outputs;
};

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

void TaskSystem::ExecuteTask(int i, bool repeat_until_fail) {
  const Task& task = tasks[i];
  LOG() << "Executing" << task;
  QUuid exec_id = QUuid::createUuid();
  active_outputs[exec_id] = TaskExecutionOutput();
  TaskExecution& exec = active_executions[exec_id];
  exec.id = exec_id;
  exec.start_time = QDateTime::currentDateTime();
  exec.task_id = task.id;
  exec.task_name = GetName(task);
  Ptr<RunTaskCommand> cmd = Ptr<RunExecutableCommand>::create(task);
  if (repeat_until_fail) {
    cmd = Ptr<RunUntilFailCommand>::create(cmd, exec_id, active_outputs);
  }
  active_commands[exec_id] = cmd;
  QObject::connect(
      cmd.get(), &RunTaskCommand::outputWritten, this,
      [this, exec_id](const QString& output, bool is_stderr) {
        AppendToExecutionOutput(exec_id, output, is_stderr);
      },
      Qt::QueuedConnection);
  QObject::connect(
      cmd.get(), &RunTaskCommand::finished, this,
      [this, exec_id](int exit_code) { FinishExecution(exec_id, exit_code); },
      Qt::QueuedConnection);
  cmd->Start();
}

void TaskSystem::KillAllTasks() {
  LOG() << "Killing all tasks";
  for (Ptr<RunTaskCommand> cmd : active_commands.values()) {
    cmd->Cancel(true);
  }
  active_executions.clear();
  active_outputs.clear();
  active_commands.clear();
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

void TaskSystem::AppendToExecutionOutput(QUuid id, const QString& data,
                                         bool is_stderr) {
  if (!active_outputs.contains(id)) {
    return;
  }
  TaskExecutionOutput& exec_output = active_outputs[id];
  if (is_stderr) {
    int lines_before = exec_output.output.count('\n');
    for (int i = 0; i < data.count('\n'); i++) {
      exec_output.stderr_line_indices.insert(i + lines_before);
    }
  }
  exec_output.output += data;
  emit executionOutputChanged(id);
}

void TaskSystem::FinishExecution(QUuid id, int exit_code) {
  if (!active_executions.contains(id)) {
    return;
  }
  TaskExecution& exec = active_executions[id];
  exec.exit_code = exit_code;
  LOG() << "Task execution" << id << "finished with code" << exit_code;
  TaskExecutionOutput& exec_output = active_outputs[id];
  const Project& project = Application::Get().project.GetCurrentProject();
  QStringList indices;
  for (int i : exec_output.stderr_line_indices) {
    indices.append(QString::number(i));
  }
  Database::ExecCmdAsync(
      "INSERT INTO task_execution VALUES(?,?,?,?,?,?,?,?)",
      {exec.id, project.id, exec.start_time, exec.task_id, exec.task_name,
       *exec.exit_code, indices.join(','), exec_output.output});
  active_executions.remove(id);
  active_outputs.remove(id);
  active_commands.remove(id);
  emit executionFinished(id);
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
  if (active_outputs.contains(execution_id)) {
    callback(active_outputs[execution_id]);
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

bool TaskSystem::IsExecutionRunning(QUuid execution_id) const {
  return active_executions.contains(execution_id);
}

void TaskSystem::CancelExecution(QUuid execution_id, bool forcefully) {
  if (!active_commands.contains(execution_id)) {
    return;
  }
  LOG() << "Attempting to cancel execution" << execution_id
        << "forcefully:" << forcefully;
  active_commands[execution_id]->Cancel(forcefully);
}

static TaskId ReadTaskId(QSqlQuery& query) { return query.value(0).toString(); }

void TaskSystem::FindTasks() {
  Application& app = Application::Get();
  QString project_path = app.project.GetCurrentProject().path;
  app.RunIOTask<QList<Task>>(
      this,
      [project_path] {
        QList<Task> results;
        QDirIterator it(project_path, QDir::Files | QDir::Executable,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
          QString path = it.next();
          path.replace(project_path, ".");
          Task task("exec:" + path);
          task.executable.path = path;
          results.append(task);
        }
        std::sort(results.begin(), results.end(),
                  [](const Task& a, const Task& b) { return a.id < b.id; });
        // Move executed tasks to the beginning so the most recently executed
        // task is first.
        QList<TaskId> task_ids = Database::ExecQueryAndRead<TaskId>(
            "SELECT task_id, MAX(start_time) as start_time "
            "FROM task_execution "
            "GROUP BY task_id "
            "ORDER BY start_time ASC",
            ReadTaskId);
        for (const TaskId& id : task_ids) {
          int index = -1;
          for (int i = 0; i < results.size(); i++) {
            if (results[i].id == id) {
              index = i;
              break;
            }
          }
          if (index >= 0) {
            results.move(index, 0);
          }
        }
        return results;
      },
      [this](QList<Task> results) {
        tasks = std::move(results);
        emit taskListRefreshed();
      });
}

const QList<Task>& TaskSystem::GetTasks() const { return tasks; }
