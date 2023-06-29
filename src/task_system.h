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
  std::optional<int> exit_code;
  QSet<int> stderr_line_indices;
  QString output;

  UiIcon GetStatusAsIcon() const;
  bool operator==(const TaskExecution& another) const;
  bool operator!=(const TaskExecution& another) const;
};

class TaskSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      QString currentTaskName READ GetCurrentTaskName NOTIFY taskListRefreshed)
 public:
  template <typename T>
  T& GetTask(int i) {
    return registry.get<T>(tasks[i]);
  }

  template <typename T>
  bool IsTask(int i) {
    return registry.all_of<T>(tasks[i]);
  }

  void KillAllTasks();
  Promise<QList<TaskExecution>> FetchExecutions(QUuid project_id) const;
  Promise<TaskExecution> FetchExecution(QUuid execution_id,
                                        bool include_output) const;
  bool IsExecutionRunning(QUuid execution_id) const;
  void FindTasks();
  void ClearTasks();
  QString GetTaskName(int i) const;
  QString GetTaskDetails(int i) const;
  QString GetTaskIcon(int i) const;
  int GetTaskCount() const;
  QString GetCurrentTaskName() const;
  void SetSelectedExecutionId(QUuid id);
  QUuid GetSelectedExecutionId() const;
  void Initialize();

  int history_limit;

 public slots:
  void executeTask(int i, bool repeat_until_fail = false);
  void cancelSelectedExecution(bool forcefully);

 signals:
  void executionOutputChanged(QUuid exec_id);
  void executionFinished(QUuid exec_id);
  void taskListRefreshed();
  void selectedExecutionChanged();

 private:
  Promise<int> RunTask(const TaskId& id, entt::entity e);
  Promise<int> RunTaskUntilFail(const TaskId& id, entt::entity e);
  Promise<int> RunExecutableTask(entt::entity task_e, entt::entity exec_e);
  Promise<int> RunCmakeTask(entt::entity task_e, entt::entity exec_e);
  Promise<int> RunCmakeTargetTask(entt::entity task_e, entt::entity exec_e);
  Promise<int> RunProcess(entt::entity e, const QString& exe,
                          const QStringList& args = {});
  void AppendToExecutionOutput(entt::entity entity, bool is_stderr);
  void AppendToExecutionOutput(entt::entity entity, QString data,
                               bool is_stderr);
  void FinishExecution(entt::entity entity, int exit_code);
  const TaskExecution* FindExecutionById(QUuid id) const;
  bool FindTaskById(const TaskId& id, entt::entity& entity) const;

  entt::registry registry;
  QList<entt::entity> tasks;
  QUuid selected_execution_id;
};
