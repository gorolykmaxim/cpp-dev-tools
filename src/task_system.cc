#include "task_system.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "path.h"
#include "theme.h"

#define LOG() qDebug() << "[TaskSystem]"

bool TaskExecution::IsNull() const { return id.isNull(); }

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

TaskContext TaskSystem::ReadContextFromSql(QSqlQuery& sql) {
  TaskContext context;
  context.history_limit = sql.value(0).toInt();
  context.run_with_console_on_win = sql.value(1).toBool();
  return context;
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
    exec.task_data = query.value(4).toByteArray();
    exec.exit_code = query.value(5).toInt();
    if (include_output) {
      QString indices = query.value(6).toString();
      for (const QString& i : indices.split(',', Qt::SkipEmptyParts)) {
        exec.stderr_line_indices.insert(i.toInt());
      }
      exec.output = query.value(7).toString();
    }
    return exec;
  };
}

void TaskSystem::LoadLastTaskExecution() {
  QUuid id = Application::Get().project.GetCurrentProject().id;
  IoTask::Run<QList<TaskExecution>>(
      this,
      [id] {
        return Database::ExecQueryAndRead(
            "SELECT id, start_time, task_id, task_name, task_data, exit_code "
            "FROM task_execution WHERE project_id = ? ORDER BY start_time DESC",
            MakeReadExecutionFromSql(false), {id});
      },
      [this](QList<TaskExecution> execs) {
        if (!execs.isEmpty()) {
          last_execution = execs.constFirst();
          LOG() << "Loaded task of last execution:" << last_execution.task_id;
          emit currentTaskChanged();
        } else {
          LOG() << "No last task execution was found";
        }
      });
}

void TaskSystem::ClearLastTaskExecution() {
  LOG() << "Resetting last execution";
  last_execution = TaskExecution{};
  emit currentTaskChanged();
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
  for (int i : std::as_const(exec.stderr_line_indices)) {
    indices.append(QString::number(i));
  }
  QList<Database::Cmd> cmds;
  cmds.append(Database::Cmd(
      "INSERT INTO task_execution VALUES(?,?,?,?,?,?,?,?,?)",
      {exec.id, project.id, exec.start_time, exec.task_id, exec.task_name,
       exec.task_data, *exec.exit_code, indices.join(','), exec.output}));
  if (context.history_limit > 0) {
    cmds.append(
        Database::Cmd("DELETE FROM task_execution WHERE id NOT IN (SELECT id "
                      "FROM task_execution ORDER BY start_time DESC LIMIT ?)",
                      {context.history_limit}));
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
        "SELECT id, start_time, task_id, task_name, task_data, exit_code FROM "
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
    QString query =
        "SELECT id, start_time, task_id, task_name, task_data, exit_code";
    if (include_output) {
      query += ", stderr_line_indices, output";
    }
    query += " FROM task_execution WHERE id=?";
    QList<TaskExecution> results = Database::ExecQueryAndRead<TaskExecution>(
        query, MakeReadExecutionFromSql(include_output), {execution_id});
    return results.isEmpty() ? TaskExecution() : results[0];
  });
}

QList<TaskExecution> TaskSystem::GetActiveExecutions() const {
  QList<TaskExecution> execs;
  for (auto [_, exec] : registry.view<const TaskExecution>().each()) {
    execs.append(exec);
  }
  return execs;
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

void TaskSystem::RunExecution(entt::entity entity, bool repeat_until_fail,
                              const QString& view) {
  auto& task_id = registry.get<TaskId>(entity);
  LOG() << "Executing" << task_id << "repeat until fail:" << repeat_until_fail;
  auto& exec = registry.emplace<TaskExecution>(entity);
  exec.id = QUuid::createUuid();
  exec.start_time = QDateTime::currentDateTime();
  exec.task_id = task_id;
  exec.task_name = GetTaskName(registry, entity);
  if (registry.any_of<CmakeTask>(entity)) {
    const auto& t = registry.get<CmakeTask>(entity);
    QJsonObject o;
    o["source_path"] = t.source_path;
    o["build_path"] = t.build_path;
    exec.task_data = QJsonDocument(o).toJson();
  } else if (registry.any_of<CmakeTargetTask>(entity)) {
    const auto& t = registry.get<CmakeTargetTask>(entity);
    QJsonObject o;
    o["build_folder"] = t.build_folder;
    o["target_name"] = t.target_name;
    o["executable"] = t.executable;
    QJsonArray args;
    for (const QString& arg : t.executable_args) {
      args.append(arg);
    }
    o["executable_args"] = args;
    o["run_after_build"] = t.run_after_build;
    exec.task_data = QJsonDocument(o).toJson();
  } else if (registry.any_of<ExecutableTask>(entity)) {
    const auto& t = registry.get<ExecutableTask>(entity);
    QJsonObject o;
    o["path"] = t.path;
    QJsonArray args;
    for (const QString& arg : t.args) {
      args.append(arg);
    }
    o["args"] = args;
    exec.task_data = QJsonDocument(o).toJson();
  } else {
    qFatal() << "Failed to execute task" << task_id << "of unknown type";
  }
  registry.emplace<QProcess>(entity);
  Promise<int> proc;
  if (repeat_until_fail) {
    proc = RunTaskUntilFail(entity);
  } else {
    proc = RunTask(entity);
  }
  proc.Then(this, [this, entity](int exit_code) {
    FinishExecution(entity, exit_code);
  });
  last_execution = exec;
  emit currentTaskChanged();
  SetSelectedExecutionId(exec.id);
  Application::Get().view.SetCurrentView(view);
}

Promise<int> TaskSystem::RunTask(entt::entity e) {
  if (registry.any_of<ExecutableTask>(e)) {
    return RunExecutableTask(e);
  } else if (registry.any_of<CmakeTask>(e)) {
    return RunCmakeTask(e);
  } else if (registry.any_of<CmakeTargetTask>(e)) {
    return RunCmakeTargetTask(e);
  }
  return Promise<int>(-1);
}

Promise<int> TaskSystem::RunTaskUntilFail(entt::entity e) {
  return RunTask(e).Then<int>(this, [e, this](int exit_code) {
    if (exit_code != 0) {
      return Promise<int>(exit_code);
    } else {
      auto& exec = registry.get<TaskExecution>(e);
      exec.output.clear();
      exec.stderr_line_indices.clear();
      return RunTaskUntilFail(e);
    }
  });
}

Promise<int> TaskSystem::RunExecutableTask(entt::entity e) {
  auto& t = registry.get<ExecutableTask>(e);
  return RunProcess(e, t.path, t.args);
}

void TaskSystem::CreateCmakeQueryFilesSync(const QString& path) {
  QString cmake_query = path + "/.cmake/api/v1/query";
  if (!QFile::exists(cmake_query)) {
    QDir().mkpath(cmake_query);
  }
  for (const QString& query :
       {cmake_query + "/cmakeFiles-v1", cmake_query + "/codemodel-v2"}) {
    if (!QFile::exists(query)) {
      LOG() << "Creating CMake query file" << query;
      QFile(query).open(QFile::WriteOnly);
    }
  }
}

QString TaskSystem::GetTaskName(const entt::registry& registry,
                                entt::entity e) {
  if (registry.any_of<ExecutableTask>(e)) {
    auto& t = registry.get<const ExecutableTask>(e);
    return Path::GetFileName(t.path);
  } else if (registry.any_of<CmakeTask>(e)) {
    auto& t = registry.get<CmakeTask>(e);
    return "CMake " + t.source_path;
  } else if (registry.any_of<CmakeTargetTask>(e)) {
    auto& t = registry.get<CmakeTargetTask>(e);
    QString result = "Build ";
    if (t.run_after_build) {
      result += "& Run ";
    }
    return result + t.target_name;
  } else {
    return registry.get<TaskId>(e);
  }
}

void TaskSystem::RunTaskOfExecution(const TaskExecution& exec,
                                    bool repeat_until_fail, const QString& view,
                                    const QStringList& executable_args) {
  if (exec.IsNull()) {
    return;
  }
  LOG() << "Running task of execution" << exec.id;
  QJsonDocument d = QJsonDocument::fromJson(exec.task_data);
  if (exec.task_id.startsWith("cmake:")) {
    CmakeTask t{
        d["source_path"].toString(),
        d["build_path"].toString(),
    };
    RunTask(exec.task_id, t, repeat_until_fail, view);
  } else if (exec.task_id.startsWith("cmake-target:")) {
    CmakeTargetTask t;
    t.build_folder = d["build_folder"].toString();
    t.target_name = d["target_name"].toString();
    t.executable = d["executable"].toString();
    t.run_after_build = d["run_after_build"].toBool();
    if (executable_args.isEmpty()) {
      for (const QJsonValue& arg : d["executable_args"].toArray()) {
        t.executable_args.append(arg.toString());
      }
    } else {
      for (const QString& arg : executable_args) {
        if (!t.executable_args.contains(arg)) {
          t.executable_args.append(arg);
        }
      }
    }
    RunTask(exec.task_id, t, repeat_until_fail, view);
  } else if (exec.task_id.startsWith("exec:")) {
    ExecutableTask t;
    t.path = d["path"].toString();
    if (executable_args.isEmpty()) {
      for (const QJsonValue& arg : d["args"].toArray()) {
        t.args.append(arg.toString());
      }
    } else {
      for (const QString& arg : executable_args) {
        if (!t.args.contains(arg)) {
          t.args.append(arg);
        }
      }
    }
    RunTask(exec.task_id, t, repeat_until_fail, view);
  } else {
    qFatal() << "Failed to execute task" << exec.task_id << "of unknown type";
  }
}

Promise<int> TaskSystem::RunCmakeTask(entt::entity e) {
  auto& t = registry.get<CmakeTask>(e);
  auto exists = QSharedPointer<bool>::create(false);
  return IoTask::Run([t, exists] {
           *exists = !QDir(t.build_path).isEmpty();
           if (!*exists) {
             CreateCmakeQueryFilesSync(t.build_path);
           }
         })
      .Then<int>(this,
                 [t, this, e]() {
                   return RunProcess(e, "cmake",
                                     {"-B", t.build_path, "-S", t.source_path});
                 })
      .Then<int>(this, [t, exists](int exit_code) {
        if (exit_code != 0 && !*exists) {
          // In case the build folder didn't exist before this invocation and we
          // are the ones who created it, if the cmake command fails - we need
          // to clean the folder up. Otherwise we would just keep on
          // uncontrollably creating new build folders.
          IoTask::Run([t] { QDir(t.build_path).removeRecursively(); });
        }
        return Promise<int>(exit_code);
      });
}

Promise<int> TaskSystem::RunCmakeTargetTask(entt::entity e) {
  auto& t = registry.get<CmakeTargetTask>(e);
  Promise<int> r =
      RunProcess(e, "cmake", {"--build", t.build_folder, "-t", t.target_name});
  if (t.run_after_build) {
    r = r.Then<int>(this, [this, e, t](int code) {
      if (code != 0) {
        return Promise<int>(code);
      }
      return RunProcess(e, t.build_folder + t.executable, t.executable_args);
    });
  }
  return r;
}

Promise<int> TaskSystem::RunProcess(entt::entity e, const QString& exe,
                                    const QStringList& args) {
  auto promise = QSharedPointer<QPromise<int>>::create();
  auto& p = registry.get<QProcess>(e);
  p.setProgram(exe);
  p.setArguments(args);
  QString cmd = exe;
  if (!args.isEmpty()) {
    cmd += ' ' + args.join(' ');
  }
  AppendToExecutionOutput(e, cmd + '\n', false);
  LOG() << "Running" << cmd;
#ifdef WIN32
  if (context.run_with_console_on_win) {
    p.setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments* args) {
          args->flags |= CREATE_NEW_CONSOLE;
        });
  }
#endif
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

QString TaskSystem::GetCurrentTaskName() const {
  return last_execution.task_name;
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
  context =
      Database::ExecQueryAndReadSync<TaskContext>(
          "SELECT history_limit, run_with_console_on_win FROM task_context",
          &TaskSystem::ReadContextFromSql)
          .constFirst();
  LOG() << "Task history limit:" << context.history_limit
        << "run with console on Windows:" << context.run_with_console_on_win;
}

const TaskExecution& TaskSystem::GetLastExecution() const {
  return last_execution;
}

TaskId CmakeTask::GetId() const {
  return "cmake:" + source_path + ':' + build_path;
}
