#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QFuture>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QSqlQuery>
#include <QString>
#include <QUuid>
#include <entt.hpp>
#include <optional>

#include "ui_icon.h"

struct ExecutableTask {
  QString path;
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

using ProcessExitCallback = std::function<void(int, QProcess::ExitStatus)>;
using ProcessPtr = QSharedPointer<QProcess>;

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
  QFuture<QList<TaskExecution>> FetchExecutions(QUuid project_id) const;
  QFuture<TaskExecution> FetchExecution(QUuid execution_id,
                                        bool include_output) const;
  bool IsExecutionRunning(QUuid execution_id) const;
  void FindTasks();
  void ClearTasks();
  QString GetTaskName(int i) const;
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
  ProcessExitCallback CreateExecutableProcess(entt::entity entity,
                                              int task_index);
  ProcessExitCallback CreateRepeatableProcess(entt::entity entity,
                                              ProcessExitCallback&& cb);
  void SetupAndRunProcess(entt::entity entity, ProcessExitCallback&& cb);
  void AppendToExecutionOutput(entt::entity entity, bool is_stderr);
  void AppendToExecutionOutput(entt::entity entity, QString data,
                               bool is_stderr);
  void FinishExecution(entt::entity entity, int exit_code);
  const TaskExecution* FindExecutionById(QUuid id) const;

  entt::registry registry;
  QList<entt::entity> tasks;
  QUuid selected_execution_id;
};
