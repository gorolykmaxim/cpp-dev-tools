#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QSqlQuery>
#include <QString>
#include <QUuid>
#include <entt.hpp>
#include <optional>

#include "promise.h"
#include "ui_icon.h"

struct ExecutableTask {
  QString path;
};

struct CmakeTask {
  QString source_path;
  QString build_path;
};

struct CmakeTargetTask {
  QString build_folder;
  QString target_name;
  QString executable;
  bool run_after_build = false;
};

typedef QString TaskId;

struct TaskExecution {
  QUuid id;
  QDateTime start_time;
  TaskId task_id;
  QString task_name;
  QByteArray task_data;
  std::optional<int> exit_code;
  QSet<int> stderr_line_indices;
  QString output;

  bool IsNull() const;
  UiIcon GetStatusAsIcon() const;
  bool operator==(const TaskExecution& another) const;
  bool operator!=(const TaskExecution& another) const;
};

struct TaskContext {
  int history_limit;
  bool run_with_console_on_win;
};

class TaskSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      QString currentTaskName READ GetCurrentTaskName NOTIFY currentTaskChanged)
 public:
  static TaskContext ReadContextFromSql(QSqlQuery& sql);
  static void CreateCmakeQueryFilesSync(const QString& path);
  static QString GetTaskName(const entt::registry& registry, entt::entity e);

  template <typename T>
  void RunTask(const TaskId& id, T t, bool repeat_until_fail) {
    entt::entity e = registry.create();
    registry.emplace<TaskId>(e, id);
    registry.emplace<T>(e, t);
    RunExecution(e, repeat_until_fail);
  }

  void RunTaskOfExecution(const TaskExecution& exec, bool repeat_until_fail);
  void KillAllTasks();
  void LoadLastTaskExecution();
  void ClearLastTaskExecution();
  const TaskExecution* FindExecutionById(QUuid id) const;
  Promise<QList<TaskExecution>> FetchExecutions(QUuid project_id) const;
  Promise<TaskExecution> FetchExecution(QUuid execution_id,
                                        bool include_output) const;
  QList<TaskExecution> GetActiveExecutions() const;
  QString GetCurrentTaskName() const;
  void SetSelectedExecutionId(QUuid id);
  QUuid GetSelectedExecutionId() const;
  void Initialize();
  const TaskExecution& GetLastExecution() const;

  TaskContext context;

 public slots:
  void cancelSelectedExecution(bool forcefully);

 signals:
  void executionOutputChanged(QUuid exec_id);
  void executionFinished(QUuid exec_id);
  void currentTaskChanged();
  void selectedExecutionChanged();

 private:
  void RunExecution(entt::entity e, bool repeat_until_fail);
  Promise<int> RunTask(entt::entity e);
  Promise<int> RunTaskUntilFail(entt::entity e);
  Promise<int> RunExecutableTask(entt::entity e);
  Promise<int> RunCmakeTask(entt::entity e);
  Promise<int> RunCmakeTargetTask(entt::entity e);
  Promise<int> RunProcess(entt::entity e, const QString& exe,
                          const QStringList& args = {});
  void AppendToExecutionOutput(entt::entity entity, bool is_stderr);
  void AppendToExecutionOutput(entt::entity entity, QString data,
                               bool is_stderr);
  void FinishExecution(entt::entity entity, int exit_code);

  entt::registry registry;
  QUuid selected_execution_id;
  TaskExecution last_execution;
};
