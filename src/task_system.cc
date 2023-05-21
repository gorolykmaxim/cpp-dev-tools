#include "task_system.h"

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "path.h"
#include "theme.h"

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

QString TaskSystem::GetName(const Task& task) {
  if (!task.executable.IsNull()) {
    return Path::GetFileName(task.executable.path);
  } else {
    return task.id;
  }
}

void TaskSystem::executeTask(int i, bool repeat_until_fail) {
  if (i < 0 || i >= tasks.size()) {
    return;
  }
  const Task& task = tasks[i];
  LOG() << "Executing" << task;
  QUuid exec_id = QUuid::createUuid();
  TaskExecution& exec = active_executions[exec_id];
  exec.id = exec_id;
  exec.start_time = QDateTime::currentDateTime();
  exec.task_id = task.id;
  exec.task_name = GetName(task);
  ProcessExitCallback cb = CreateExecutableProcess(exec_id, task);
  if (repeat_until_fail) {
    cb = CreateRepeatableProcess(exec_id, std::move(cb));
  }
  SetupAndRunProcess(exec_id, std::move(cb));
  tasks.move(i, 0);
  emit taskListRefreshed();
  SetSelectedExecutionId(exec_id);
  Application::Get().view.SetCurrentView("TaskExecution.qml");
}

void TaskSystem::KillAllTasks() {
  LOG() << "Killing all tasks";
  for (ProcessPtr proc : qAsConst(active_processes)) {
    proc->kill();
  }
  active_executions.clear();
  active_processes.clear();
}

static std::function<TaskExecution(QSqlQuery&)> MakeReadExecutionFromSql(
    bool include_output) {
  return [include_output](QSqlQuery& query) {
    TaskExecution exec;
    exec.id = query.value(0).toUuid();
    exec.start_time = query.value(1).toDateTime();
    exec.task_id = query.value(2).toString();
    exec.task_name = query.value(3).toString();
    exec.exit_code = query.value(4).toInt();
    if (include_output) {
      QString indices = query.value(5).toString();
      for (const QString& i : indices.split(',', Qt::SkipEmptyParts)) {
        exec.stderr_line_indices.insert(i.toInt());
      }
      exec.output = query.value(6).toString();
    }
    return exec;
  };
}

void TaskSystem::AppendToExecutionOutput(QUuid id, bool is_stderr) {
  if (!active_processes.contains(id)) {
    return;
  }
  ProcessPtr proc = active_processes[id];
  QString data;
  if (is_stderr) {
    data = proc->readAllStandardError();
  } else {
    data = proc->readAllStandardOutput();
  }
  AppendToExecutionOutput(id, data, is_stderr);
}

void TaskSystem::AppendToExecutionOutput(QUuid id, QString data,
                                         bool is_stderr) {
  if (!active_executions.contains(id)) {
    return;
  }
  TaskExecution& exec = active_executions[id];
  data.remove('\r');
  if (is_stderr) {
    int lines_before = exec.output.count('\n');
    for (int i = 0; i < data.count('\n'); i++) {
      exec.stderr_line_indices.insert(i + lines_before);
    }
  }
  exec.output += data;
  emit executionOutputChanged(id);
}

void TaskSystem::FinishExecution(QUuid id, int exit_code) {
  if (!active_executions.contains(id)) {
    return;
  }
  TaskExecution& exec = active_executions[id];
  exec.exit_code = exit_code;
  LOG() << "Task execution" << id << "finished with code" << exit_code;
  const Project& project = Application::Get().project.GetCurrentProject();
  QStringList indices;
  for (int i : qAsConst(exec.stderr_line_indices)) {
    indices.append(QString::number(i));
  }
  QList<Database::Cmd> cmds;
  cmds.append(Database::Cmd(
      "INSERT INTO task_execution VALUES(?,?,?,?,?,?,?,?)",
      {exec.id, project.id, exec.start_time, exec.task_id, exec.task_name,
       *exec.exit_code, indices.join(','), exec.output}));
  if (history_limit > 0) {
    cmds.append(
        Database::Cmd("DELETE FROM task_execution WHERE id NOT IN (SELECT id "
                      "FROM task_execution ORDER BY start_time DESC LIMIT ?)",
                      {history_limit}));
  }
  Database::ExecCmdsAsync(cmds);
  active_executions.remove(id);
  active_processes.remove(id);
  emit executionFinished(id);
}

QFuture<QList<TaskExecution>> TaskSystem::FetchExecutions(
    QUuid project_id) const {
  LOG() << "Fetching executions for project" << project_id;
  QList<TaskExecution> execs = active_executions.values();
  std::sort(execs.begin(), execs.end(),
            [](const TaskExecution& a, const TaskExecution& b) {
              return a.start_time < b.start_time;
            });
  return IoTask::Run<QList<TaskExecution>>([project_id, execs] {
    QList<TaskExecution> result = Database::ExecQueryAndRead<TaskExecution>(
        "SELECT id, start_time, task_id, task_name, exit_code FROM "
        "task_execution "
        "WHERE project_id=? ORDER BY start_time",
        MakeReadExecutionFromSql(false), {project_id});
    for (const TaskExecution& exec : execs) {
      // Due to possible races between threads and callback scheduling
      // on the main thread, some of "execs" might have already been
      // inserted into the database and might be included in "result".
      if (!result.contains(exec)) {
        result.append(exec);
      }
    }
    return result;
  });
}

QFuture<TaskExecution> TaskSystem::FetchExecution(QUuid execution_id,
                                                  bool include_output) const {
  LOG() << "Fetching execution" << execution_id
        << "including output:" << include_output;
  if (active_executions.contains(execution_id)) {
    return QtFuture::makeReadyFuture(active_executions[execution_id]);
  }
  return IoTask::Run<TaskExecution>([execution_id, include_output] {
    QString query = "SELECT id, start_time, task_id, task_name, exit_code";
    if (include_output) {
      query += ", stderr_line_indices, output";
    }
    query += " FROM task_execution WHERE id=?";
    QList<TaskExecution> results = Database::ExecQueryAndRead<TaskExecution>(
        query, MakeReadExecutionFromSql(include_output), {execution_id});
    return results.isEmpty() ? TaskExecution() : results[0];
  });
}

bool TaskSystem::IsExecutionRunning(QUuid execution_id) const {
  return active_executions.contains(execution_id);
}

void TaskSystem::cancelSelectedExecution(bool forcefully) {
  if (!active_processes.contains(selected_execution_id)) {
    return;
  }
  LOG() << "Attempting to cancel execution" << selected_execution_id
        << "forcefully:" << forcefully;
  ProcessPtr p = active_processes[selected_execution_id];
  if (forcefully) {
    p->kill();
  } else {
    p->terminate();
  }
}

ProcessExitCallback TaskSystem::CreateExecutableProcess(QUuid id,
                                                        const Task& task) {
  ProcessPtr p = ProcessPtr::create();
  p->setProgram(task.executable.path);
  active_processes[id] = p;
  AppendToExecutionOutput(id, task.executable.path + '\n', false);
  return [id, this](int exit_code, QProcess::ExitStatus) {
    FinishExecution(id, exit_code);
  };
}

ProcessExitCallback TaskSystem::CreateRepeatableProcess(
    QUuid id, ProcessExitCallback&& cb) {
  return [id, cb = std::move(cb), this](int exit_code,
                                        QProcess::ExitStatus status) {
    if (exit_code != 0) {
      cb(exit_code, status);
    } else {
      TaskExecution& exec = active_executions[id];
      exec.output.clear();
      exec.stderr_line_indices.clear();
      ProcessPtr p = active_processes[id];
      AppendToExecutionOutput(id, p->program() + '\n', false);
      p->start();
    }
  };
}

void TaskSystem::SetupAndRunProcess(QUuid id, ProcessExitCallback&& cb) {
  ProcessPtr p = active_processes[id];
  connect(
      p.get(), &QProcess::readyReadStandardError, this,
      [id, this] { AppendToExecutionOutput(id, true); }, Qt::QueuedConnection);
  connect(
      p.get(), &QProcess::readyReadStandardOutput, this,
      [id, this] { AppendToExecutionOutput(id, false); }, Qt::QueuedConnection);
  connect(
      p.get(), &QProcess::errorOccurred, this,
      [id, this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart ||
            !active_processes.contains(id)) {
          return;
        }
        AppendToExecutionOutput(id, "Failed to start executable\n", true);
        emit active_processes[id]->finished(-1);
      },
      Qt::QueuedConnection);
  connect(p.get(), &QProcess::finished, this, std::move(cb),
          Qt::QueuedConnection);
  p->start();
}

static TaskExecution ReadTaskExecutionStartTime(QSqlQuery& query) {
  TaskExecution exec;
  exec.task_id = query.value(0).toString();
  exec.start_time = query.value(1).toDateTime();
  return exec;
}

void TaskSystem::FindTasks() {
  QString project_path = Application::Get().project.GetCurrentProject().path;
  LOG() << "Refreshing task list";
  QList<TaskExecution> active_execs = active_executions.values();
  IoTask::Run<QList<Task>>(
      this,
      [project_path, active_execs] {
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
        QList<TaskExecution> execs = Database::ExecQueryAndRead<TaskExecution>(
            "SELECT task_id, MAX(start_time) as start_time "
            "FROM task_execution "
            "GROUP BY task_id "
            "ORDER BY start_time ASC",
            ReadTaskExecutionStartTime);
        // Merge active executions with finished ones to account for start times
        // of executions that are still running.
        for (const TaskExecution& active : active_execs) {
          bool updated = false;
          for (TaskExecution& exec : execs) {
            if (exec.task_id == active.task_id &&
                active.start_time > exec.start_time) {
              exec.start_time = active.start_time;
              updated = true;
              break;
            }
          }
          if (!updated) {
            execs.append(active);
          }
        }
        std::sort(execs.begin(), execs.end(),
                  [](const TaskExecution& a, const TaskExecution& b) {
                    return a.start_time < b.start_time;
                  });
        for (const TaskExecution& exec : execs) {
          int index = -1;
          for (int i = 0; i < results.size(); i++) {
            if (results[i].id == exec.task_id) {
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

void TaskSystem::ClearTasks() {
  tasks.clear();
  emit taskListRefreshed();
}

const QList<Task>& TaskSystem::GetTasks() const { return tasks; }

QString TaskSystem::GetCurrentTaskName() const {
  if (tasks.isEmpty()) {
    return "";
  }
  return GetName(tasks[0]);
}

void TaskSystem::SetSelectedExecutionId(QUuid id) {
  LOG() << "Selected execution" << id;
  selected_execution_id = id;
  emit selectedExecutionChanged();
}

QUuid TaskSystem::GetSelectedExecutionId() const {
  return selected_execution_id;
}

void TaskSystem::Initialize() {
  LOG() << "Initializing";
  history_limit =
      Database::ExecQueryAndReadSync<int>(
          "SELECT history_limit FROM task_context", &Database::ReadIntFromSql)
          .constFirst();
  LOG() << "Task history limit:" << history_limit;
}

UiIcon TaskSystem::GetStatusAsIcon(const TaskExecution& exec) {
  Theme theme;
  UiIcon icon;
  if (!exec.exit_code) {
    icon.icon = "autorenew";
    icon.color = "green";
  } else {
    if (*exec.exit_code == 0) {
      icon.icon = "check";
      icon.color = theme.kColorText;
    } else {
      icon.icon = "error";
      icon.color = "red";
    }
  }
  return icon;
}
