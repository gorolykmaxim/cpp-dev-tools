#include "task_list_model.h"

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "path.h"

#define LOG() qDebug() << "[TaskListModel]"

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
    TaskSystem::CreateCmakeQueryFilesSync(path);
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
  // Create tasks for running CMake build generation
  for (const QString& path : info.cmake_source_folders) {
    for (const QString& build_folder : cmake_source_to_builds[path]) {
      entt::entity entity = registry.create();
      auto& t = registry.emplace<CmakeTask>(entity);
      t.source_path = path;
      t.build_path = build_folder;
      registry.emplace<TaskId>(entity, t.GetId());
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

static TaskExecution ReadTaskExecutionStartTime(QSqlQuery& query) {
  TaskExecution exec;
  exec.task_id = query.value(0).toString();
  exec.start_time = query.value(1).toDateTime();
  return exec;
}

static void SortFoundTasks(entt::registry& registry, QList<entt::entity>& tasks,
                           const QList<TaskExecution>& active_execs,
                           QUuid project_id) {
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
      "WHERE project_id=? "
      "GROUP BY task_id "
      "ORDER BY start_time ASC",
      ReadTaskExecutionStartTime, {project_id});
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

template <typename T>
static void CopyTaskComp(entt::registry& source_r, entt::registry& target_r,
                         entt::entity source_e, entt::entity target_e) {
  if (source_r.all_of<T>(source_e)) {
    target_r.emplace<T>(target_e, source_r.get<T>(source_e));
  }
}

TaskListModel::TaskListModel(QObject* parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "subTitle"}, {2, "icon"}});
  searchable_roles = {0, 1};
  SetEmptyListPlaceholder("No tasks found");
}

void TaskListModel::displayTaskList() {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Tasks");
  const Project& project = app.project.GetCurrentProject();
  QUuid project_id = project.id;
  QString project_path = project.path;
  auto task_entities = QSharedPointer<QList<entt::entity>>::create();
  auto task_registry = QSharedPointer<entt::registry>::create();
  QList<TaskExecution> active_execs = app.task.GetActiveExecutions();
  LOG() << "Looking for tasks";
  SetPlaceholder("Looking for tasks...");
  IoTask::Run(
      this,
      [project_id, project_path, active_execs, task_entities, task_registry] {
        TasksInfo info = ScanDirectoryForTasksInfo(project_path);
        CreateExecutableTasks(info, *task_registry, *task_entities);
        CreateCmakeTasks(info, project_path, *task_registry, *task_entities);
        SortFoundTasks(*task_registry, *task_entities, active_execs,
                       project_id);
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
        Load(-1);
        SetPlaceholder();
      });
}

void TaskListModel::executeCurrentTask(bool repeat_until_fail,
                                       const QString& view,
                                       const QStringList& args) {
  Application& app = Application::Get();
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  entt::entity e = tasks[i];
  LOG() << "Executing task" << registry.get<TaskId>(e)
        << "repeat until fail:" << repeat_until_fail;
  if (registry.any_of<CmakeTask>(e)) {
    app.task.RunTask(registry.get<TaskId>(e), registry.get<CmakeTask>(e),
                     repeat_until_fail, view);
  } else if (registry.any_of<CmakeTargetTask>(e)) {
    CmakeTargetTask t = registry.get<CmakeTargetTask>(e);
    t.executable_args = args;
    app.task.RunTask(registry.get<TaskId>(e), t, repeat_until_fail, view);
  } else if (registry.any_of<ExecutableTask>(e)) {
    ExecutableTask t = registry.get<ExecutableTask>(e);
    t.args = args;
    app.task.RunTask(registry.get<TaskId>(e), t, repeat_until_fail, view);
  }
}

QVariantList TaskListModel::GetRow(int i) const {
  entt::entity e = tasks[i];
  QString name = TaskSystem::GetTaskName(registry, e);
  QString details, icon;
  if (registry.any_of<ExecutableTask>(e)) {
    auto& t = registry.get<const ExecutableTask>(e);
    details = t.path;
    icon = "code";
  } else if (registry.any_of<CmakeTask>(e)) {
    auto& t = registry.get<CmakeTask>(e);
    details = "cmake -B " + t.build_path + " -S " + t.source_path;
    icon = "change_history";
  } else if (registry.any_of<CmakeTargetTask>(e)) {
    auto& t = registry.get<CmakeTargetTask>(e);
    details = "cmake --build " + t.build_folder + " -t " + t.target_name;
    icon = "change_history";
  } else {
    details = registry.get<TaskId>(e);
    icon = "code";
  }
  return {name, details, icon};
}

int TaskListModel::GetRowCount() const { return tasks.size(); }
