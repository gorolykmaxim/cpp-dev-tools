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
    proc = RunTaskUntilFail(task_id, entity);
  } else {
    proc = RunTask(task_id, entity);
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

bool TaskSystem::FindTaskById(const TaskId& id, entt::entity& entity) const {
  for (auto [e, task_id] : registry.view<TaskId>().each()) {
    if (task_id == id) {
      entity = e;
      return true;
    }
  }
  return false;
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

Promise<int> TaskSystem::RunTask(const TaskId& id, entt::entity e) {
  entt::entity task_e;
  if (FindTaskById(id, task_e)) {
    if (registry.any_of<ExecutableTask>(task_e)) {
      return RunExecutableTask(task_e, e);
    } else if (registry.any_of<CmakeTask>(task_e)) {
      return RunCmakeTask(task_e, e);
    } else if (registry.any_of<CmakeTargetTask>(task_e)) {
      return RunCmakeTargetTask(task_e, e);
    } else {
      LOG() << "Can't execute task" << id << "with unsupported type";
    }
  } else {
    LOG() << "Can't execute task" << id << "because it no longer exists";
  }
  return Promise<int>(-1);
}

Promise<int> TaskSystem::RunTaskUntilFail(const TaskId& id, entt::entity e) {
  return RunTask(id, e).Then<int>(this, [e, this, id](int exit_code) {
    if (exit_code != 0) {
      return Promise<int>(exit_code);
    } else {
      auto& exec = registry.get<TaskExecution>(e);
      exec.output.clear();
      exec.stderr_line_indices.clear();
      return RunTaskUntilFail(id, e);
    }
  });
}

Promise<int> TaskSystem::RunExecutableTask(entt::entity task_e,
                                           entt::entity exec_e) {
  auto& t = registry.get<ExecutableTask>(task_e);
  return RunProcess(exec_e, t.path);
}

static void CreateCmakeQueryFiles(const QString& path) {
  QString cmake_query = path + "/.cmake/api/v1/query";
  if (!QFile::exists(cmake_query)) {
    QDir().mkpath(cmake_query);
  }
  for (const QString& query :
       {cmake_query + "/cmakeFiles-v1", cmake_query + "/codemodel-v2"}) {
    if (!QFile::exists(query)) {
      QFile(query).open(QFile::WriteOnly);
    }
  }
}

Promise<int> TaskSystem::RunCmakeTask(entt::entity task_e,
                                      entt::entity exec_e) {
  auto& t = registry.get<CmakeTask>(task_e);
  auto exists = QSharedPointer<bool>::create(false);
  return IoTask::Run([t, exists] {
           *exists = QFile::exists(t.build_path);
           if (!*exists) {
             CreateCmakeQueryFiles(t.build_path);
           }
         })
      .Then<int>(this,
                 [t, this, exec_e]() {
                   return RunProcess(exec_e, "cmake",
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

Promise<int> TaskSystem::RunCmakeTargetTask(entt::entity task_e,
                                            entt::entity exec_e) {
  auto& t = registry.get<CmakeTargetTask>(task_e);
  Promise<int> r = RunProcess(exec_e, "cmake",
                              {"--build", t.build_folder, "-t", t.target_name});
  if (t.run_after_build) {
    r = r.Then<int>(this, [this, exec_e, t](int code) {
      if (code != 0) {
        return Promise<int>(code);
      }
      return RunProcess(exec_e, t.build_folder + t.executable);
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

template <typename T>
static void CopyTaskComp(entt::registry& source_r, entt::registry& target_r,
                         entt::entity source_e, entt::entity target_e) {
  if (source_r.all_of<T>(source_e)) {
    target_r.emplace<T>(target_e, source_r.get<T>(source_e));
  }
}

struct TasksInfo {
  QStringList executables;
  QStringList cmake_build_folders;
  QStringList cmake_source_folders;
  QStringList cmake_cmake_file_replies;
  QStringList cmake_target_replies;
};

static void ScanFile(TasksInfo& info, const QString& root, QString path,
                     QFileInfo file_info) {
  path.replace(root, ".");
  if (file_info.isExecutable()) {
    info.executables.append(path);
  } else if (file_info.fileName() == "CMakeCache.txt") {
    info.cmake_build_folders.append(Path::GetFolderPath(path));
  } else if (file_info.fileName() == "CMakeLists.txt") {
    info.cmake_source_folders.append(Path::GetFolderPath(path));
  } else if (Path::MatchesWildcard(
                 path, "*/.cmake/api/v1/reply/cmakeFiles-v1-*.json")) {
    info.cmake_cmake_file_replies.append(path);
  } else if (Path::MatchesWildcard(path,
                                   "*/.cmake/api/v1/reply/target-*.json")) {
    info.cmake_target_replies.append(path);
  }
}

static TasksInfo ScanDirectoryForTasksInfo(const QString& directory) {
  TasksInfo info;
  // Skip such hidden top-level folders as .git to improve performance.
  QDir::Filters top_filters = QDir::AllEntries | QDir::NoDotAndDotDot;
  QFileInfoList list = QDir(directory).entryInfoList(top_filters);
  for (const QFileInfo& i : list) {
    if (!i.isDir()) {
      ScanFile(info, directory, i.filePath(), i);
    } else {
      QDir::Filters filters = QDir::Files | QDir::Hidden;
      QDirIterator it(i.filePath(), filters, QDirIterator::Subdirectories);
      while (it.hasNext()) {
        QString path = it.next();
        ScanFile(info, directory, path, it.fileInfo());
      }
    }
  }
  LOG() << directory;
  return info;
}

static void CreateExecutableTasks(const TasksInfo& info,
                                  entt::registry& registry,
                                  QList<entt::entity>& tasks) {
  for (const QString& path : info.executables) {
    entt::entity entity = registry.create();
    registry.emplace<TaskId>(entity, "exec:" + path);
    auto& t = registry.emplace<ExecutableTask>(entity);
    t.path = path;
    tasks.append(entity);
  }
}

static void CreateCmakeTasks(TasksInfo& info, const QString& project_path,
                             entt::registry& registry,
                             QList<entt::entity>& tasks) {
  // Remove build folders that are inside other build folders
  info.cmake_build_folders.removeIf([&info](const QString& p) {
    for (const QString& p1 : info.cmake_build_folders) {
      if (p != p1 && p.startsWith(p1)) {
        return true;
      }
    }
    return false;
  });
  // Create queries for the next CMake execution if they don't exist already
  for (const QString& path : info.cmake_build_folders) {
    CreateCmakeQueryFiles(path);
  }
  // Find which build directories correspond to which source directories
  QHash<QString, QStringList> cmake_source_to_builds;
  for (const QString& path : info.cmake_cmake_file_replies) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
      continue;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    QJsonObject paths = doc["paths"].toObject();
    QString build = paths["build"].toString();
    build.replace(project_path, ".");
    if (!build.endsWith('/')) {
      build += '/';
    }
    QString source = paths["source"].toString();
    source.replace(project_path, ".");
    if (!source.endsWith('/')) {
      source += '/';
    }
    cmake_source_to_builds[source].append(build);
  }
  // Determine a name for a new build folder
  QString new_build_folder = "./build/";
  int build_gen = 1;
  while (info.cmake_build_folders.contains(new_build_folder)) {
    new_build_folder = "./build-gen-" + QString::number(build_gen++) + '/';
  }
  // Create tasks for running CMake build generation
  for (const QString& path : info.cmake_source_folders) {
    QStringList build_folders = {new_build_folder};
    build_folders.append(cmake_source_to_builds[path]);
    for (const QString& build_folder : build_folders) {
      entt::entity entity = registry.create();
      registry.emplace<TaskId>(entity, "cmake:" + path + ':' + build_folder);
      auto& t = registry.emplace<CmakeTask>(entity);
      t.source_path = path;
      t.build_path = build_folder;
      tasks.append(entity);
    }
  }
  // Create tasks for building and running CMake targets
  for (const QString& path : info.cmake_target_replies) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
      continue;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (doc["type"].toString() != "EXECUTABLE") {
      continue;
    }
    QString target = doc["name"].toString();
    QString exec_path;
    QJsonArray artifacts = doc["artifacts"].toArray();
    if (!artifacts.isEmpty()) {
      exec_path = artifacts[0].toObject()["path"].toString();
    }
    QString build_folder;
    for (const QString& build_path : info.cmake_build_folders) {
      if (path.startsWith(build_path)) {
        build_folder = build_path;
        break;
      }
    }
    for (bool run_after_build : {false, true}) {
      entt::entity entity = registry.create();
      registry.emplace<TaskId>(
          entity, "cmake-target:" + target + ':' + exec_path + ':' +
                      build_folder + ':' + QString::number(run_after_build));
      auto& t = registry.emplace<CmakeTargetTask>(entity);
      t.target_name = target;
      t.build_folder = build_folder;
      t.executable = exec_path;
      t.run_after_build = run_after_build;
      tasks.append(entity);
    }
  }
}

static void SortFoundTasks(entt::registry& registry, QList<entt::entity>& tasks,
                           const QList<TaskExecution>& active_execs) {
  // First, sort tasks in their natural "by-ID" order
  std::sort(tasks.begin(), tasks.end(),
            [&registry](entt::entity a, entt::entity b) {
              return registry.get<TaskId>(a) < registry.get<TaskId>(b);
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
    for (int i = 0; i < tasks.size(); i++) {
      auto& task_id = registry.get<TaskId>(tasks.at(i));
      if (task_id == exec.task_id) {
        index = i;
        break;
      }
    }
    if (index >= 0) {
      tasks.move(index, 0);
    }
  }
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
        TasksInfo info = ScanDirectoryForTasksInfo(project_path);
        CreateExecutableTasks(info, *task_registry, *task_entities);
        CreateCmakeTasks(info, project_path, *task_registry, *task_entities);
        SortFoundTasks(*task_registry, *task_entities, active_execs);
      },
      [this, task_entities, task_registry]() {
        registry.destroy(tasks.begin(), tasks.end());
        tasks.clear();
        for (entt::entity entity : *task_entities) {
          entt::entity e = registry.create();
          tasks.append(e);
          CopyTaskComp<TaskId>(*task_registry, registry, entity, e);
          CopyTaskComp<ExecutableTask>(*task_registry, registry, entity, e);
          CopyTaskComp<CmakeTask>(*task_registry, registry, entity, e);
          CopyTaskComp<CmakeTargetTask>(*task_registry, registry, entity, e);
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

QString TaskSystem::GetTaskDetails(int i) const {
  entt::entity e = tasks[i];
  if (registry.any_of<ExecutableTask>(e)) {
    return registry.get<ExecutableTask>(e).path;
  } else if (registry.any_of<CmakeTask>(e)) {
    auto& t = registry.get<CmakeTask>(e);
    return "cmake -B " + t.build_path + " -S " + t.source_path;
  } else if (registry.any_of<CmakeTargetTask>(e)) {
    auto& t = registry.get<CmakeTargetTask>(e);
    return "cmake --build " + t.build_folder + " -t " + t.target_name;
  } else {
    return registry.get<TaskId>(e);
  }
}

QString TaskSystem::GetTaskIcon(int i) const {
  if (registry.any_of<CmakeTask, CmakeTargetTask>(tasks[i])) {
    return "change_history";
  } else {
    return "code";
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
