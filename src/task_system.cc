#include "task_system.h"

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "path.h"
#include "theme.h"

#define LOG() qDebug() << "[TaskSystem]"

UiIcon TaskExecution::GetStatusAsIcon() const {
  static const Theme kTheme;
  UiIcon icon;
  if (!exit_code) {
    icon.icon = "autorenew";
    icon.color = "green";
  } else {
    if (*exit_code == 0) {
      icon.icon = "check";
      icon.color = kTheme.kColorText;
    } else {
      icon.icon = "error";
      icon.color = "red";
    }
  }
  return icon;
}

bool TaskExecution::operator==(const TaskExecution& another) const {
  return id == another.id;
}

bool TaskExecution::operator!=(const TaskExecution& another) const {
  return !(*this == another);
}

void TaskSystem::executeTask(int i, bool repeat_until_fail) {
  if (i < 0 || i >= tasks.size()) {
    return;
  }
  auto& task_id = GetTask<TaskId>(i);
  LOG() << "Executing" << task_id;
  entt::entity entity = registry.create();
  auto& exec = registry.emplace<TaskExecution>(entity);
  exec.id = QUuid::createUuid();
  exec.start_time = QDateTime::currentDateTime();
  exec.task_id = task_id;
  exec.task_name = GetTaskName(i);
  registry.emplace<QProcess>(entity);
  Promise<int> proc;
  if (repeat_until_fail) {
    proc = RunTaskUntilFail(entity, i);
  } else {
    proc = RunTask(entity, i);
  }
  proc.Then(this, [this, entity](int exit_code) {
    FinishExecution(entity, exit_code);
  });
  tasks.move(i, 0);
  emit taskListRefreshed();
  SetSelectedExecutionId(exec.id);
  Application::Get().view.SetCurrentView("TaskExecution.qml");
}

void TaskSystem::KillAllTasks() {
  LOG() << "Killing all tasks";
  QList<entt::entity> to_destroy;
  for (auto [entity, proc] : registry.view<QProcess>().each()) {
    proc.kill();
    to_destroy.append(entity);
  }
  registry.destroy(to_destroy.begin(), to_destroy.end());
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

void TaskSystem::AppendToExecutionOutput(entt::entity entity, bool is_stderr) {
  if (!registry.all_of<QProcess>(entity)) {
    return;
  }
  auto& proc = registry.get<QProcess>(entity);
  QString data;
  if (is_stderr) {
    data = proc.readAllStandardError();
  } else {
    data = proc.readAllStandardOutput();
  }
  AppendToExecutionOutput(entity, data, is_stderr);
}

void TaskSystem::AppendToExecutionOutput(entt::entity entity, QString data,
                                         bool is_stderr) {
  if (!registry.all_of<TaskExecution>(entity)) {
    return;
  }
  auto& exec = registry.get<TaskExecution>(entity);
  data.remove('\r');
  if (is_stderr) {
    int lines_before = exec.output.count('\n');
    for (int i = 0; i < data.count('\n'); i++) {
      exec.stderr_line_indices.insert(i + lines_before);
    }
  }
  exec.output += data;
  emit executionOutputChanged(exec.id);
}

void TaskSystem::FinishExecution(entt::entity entity, int exit_code) {
  if (!registry.all_of<TaskExecution>(entity)) {
    return;
  }
  auto exec = registry.get<TaskExecution>(entity);
  exec.exit_code = exit_code;
  LOG() << "Task execution" << exec.id << "finished with code" << exit_code;
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
  registry.destroy(entity);
  emit executionFinished(exec.id);
}

const TaskExecution* TaskSystem::FindExecutionById(QUuid id) const {
  for (auto [_, exec] : registry.view<TaskExecution>().each()) {
    if (exec.id == id) {
      return &exec;
    }
  }
  return nullptr;
}

Promise<QList<TaskExecution>> TaskSystem::FetchExecutions(
    QUuid project_id) const {
  LOG() << "Fetching executions for project" << project_id;
  QList<TaskExecution> execs;
  for (auto [_, exec] : registry.view<const TaskExecution>().each()) {
    execs.append(exec);
  }
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

Promise<TaskExecution> TaskSystem::FetchExecution(QUuid execution_id,
                                                  bool include_output) const {
  LOG() << "Fetching execution" << execution_id
        << "including output:" << include_output;
  if (const TaskExecution* exec = FindExecutionById(execution_id)) {
    return Promise<TaskExecution>(*exec);
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
  return FindExecutionById(execution_id);
}

void TaskSystem::cancelSelectedExecution(bool forcefully) {
  for (auto [_, exec, proc] :
       registry.view<const TaskExecution, QProcess>().each()) {
    if (exec.id != selected_execution_id) {
      continue;
    }
    LOG() << "Attempting to cancel execution" << selected_execution_id
          << "forcefully:" << forcefully;
    if (forcefully) {
      proc.kill();
    } else {
      proc.terminate();
    }
  }
}

Promise<int> TaskSystem::RunTask(entt::entity e, int i) {
  auto& t = GetTask<const ExecutableTask>(i);
  registry.get<QProcess>(e).setProgram(t.path);
  AppendToExecutionOutput(e, t.path + '\n', false);
  return RunProcess(e);
}

Promise<int> TaskSystem::RunTaskUntilFail(entt::entity e, int i) {
  return RunTask(e, i).Then<int>(this, [e, this, i](int exit_code) {
    if (exit_code != 0) {
      return Promise<int>(exit_code);
    } else {
      auto& exec = registry.get<TaskExecution>(e);
      exec.output.clear();
      exec.stderr_line_indices.clear();
      return RunTaskUntilFail(e, i);
    }
  });
}

Promise<int> TaskSystem::RunProcess(entt::entity e) {
  auto promise = QSharedPointer<QPromise<int>>::create();
  auto& p = registry.get<QProcess>(e);
  connect(
      &p, &QProcess::readyReadStandardError, this,
      [e, this] { AppendToExecutionOutput(e, true); }, Qt::QueuedConnection);
  connect(
      &p, &QProcess::readyReadStandardOutput, this,
      [e, this] { AppendToExecutionOutput(e, false); }, Qt::QueuedConnection);
  connect(
      &p, &QProcess::errorOccurred, this,
      [e, this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !registry.all_of<QProcess>(e)) {
          return;
        }
        AppendToExecutionOutput(e, "Failed to start executable\n", true);
        emit registry.get<QProcess>(e).finished(-1);
      },
      Qt::QueuedConnection);
  connect(
      &p, &QProcess::finished, this,
      [promise](int exit_code, QProcess::ExitStatus) {
        promise->addResult(exit_code);
        promise->finish();
      },
      Qt::QueuedConnection);
  p.start();
  return promise->future();
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
  QList<TaskExecution> active_execs;
  for (auto [_, exec] : registry.view<const TaskExecution>().each()) {
    active_execs.append(exec);
  }
  auto task_entities = QSharedPointer<QList<entt::entity>>::create();
  auto task_registry = QSharedPointer<entt::registry>::create();
  IoTask::Run(
      this,
      [project_path, active_execs, task_entities, task_registry] {
        QDirIterator it(project_path, QDir::Files | QDir::Executable,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
          QString path = it.next();
          path.replace(project_path, ".");
          entt::entity entity = task_registry->create();
          task_registry->emplace<TaskId>(entity, "exec:" + path);
          auto& t = task_registry->emplace<ExecutableTask>(entity);
          t.path = path;
          task_entities->append(entity);
        }
        std::sort(task_entities->begin(), task_entities->end(),
                  [&task_registry](entt::entity a, entt::entity b) {
                    return task_registry->get<TaskId>(a) <
                           task_registry->get<TaskId>(b);
                  });
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
          for (int i = 0; i < task_entities->size(); i++) {
            auto& task_id = task_registry->get<TaskId>(task_entities->at(i));
            if (task_id == exec.task_id) {
              index = i;
              break;
            }
          }
          if (index >= 0) {
            task_entities->move(index, 0);
          }
        }
      },
      [this, task_entities, task_registry]() {
        registry.destroy(tasks.begin(), tasks.end());
        tasks.clear();
        for (entt::entity entity : *task_entities) {
          entt::entity e = registry.create();
          tasks.append(e);
          registry.emplace<TaskId>(e, task_registry->get<TaskId>(entity));
          registry.emplace<ExecutableTask>(
              e, task_registry->get<ExecutableTask>(entity));
        }
        emit taskListRefreshed();
      });
}

void TaskSystem::ClearTasks() {
  registry.destroy(tasks.begin(), tasks.end());
  tasks.clear();
  emit taskListRefreshed();
}

QString TaskSystem::GetTaskName(int i) const {
  entt::entity e = tasks[i];
  if (registry.any_of<ExecutableTask>(e)) {
    auto& t = registry.get<const ExecutableTask>(e);
    return Path::GetFileName(t.path);
  } else {
    return registry.get<TaskId>(e);
  }
}

int TaskSystem::GetTaskCount() const { return tasks.size(); }

QString TaskSystem::GetCurrentTaskName() const {
  if (tasks.isEmpty()) {
    return "";
  }
  return GetTaskName(0);
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
