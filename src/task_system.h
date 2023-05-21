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
#include <optional>

#include "ui_icon.h"

struct ExecutableTask {
  QString path;

  bool IsNull() const;
};

using TaskId = QString;

struct Task {
  TaskId id;
  ExecutableTask executable;

  explicit Task(const TaskId& id);
};

QDebug operator<<(QDebug debug, const Task& task);

struct TaskExecution {
  QUuid id;
  QDateTime start_time;
  TaskId task_id;
  QString task_name;
  std::optional<int> exit_code;
  QSet<int> stderr_line_indices;
  QString output;

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
  static QString GetName(const Task& task);
  static UiIcon GetStatusAsIcon(const TaskExecution& exec);
  void KillAllTasks();
  QFuture<QList<TaskExecution>> FetchExecutions(QUuid project_id) const;
  QFuture<TaskExecution> FetchExecution(QUuid execution_id,
                                        bool include_output) const;
  bool IsExecutionRunning(QUuid execution_id) const;
  void FindTasks();
  void ClearTasks();
  const QList<Task>& GetTasks() const;
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
  ProcessExitCallback CreateExecutableProcess(QUuid id, const Task& task);
  ProcessExitCallback CreateRepeatableProcess(QUuid id,
                                              ProcessExitCallback&& cb);
  void SetupAndRunProcess(QUuid id, ProcessExitCallback&& cb);
  void AppendToExecutionOutput(QUuid id, bool is_stderr);
  void AppendToExecutionOutput(QUuid id, QString data, bool is_stderr);
  void FinishExecution(QUuid id, int exit_code);

  QHash<QUuid, TaskExecution> active_executions;
  QHash<QUuid, ProcessPtr> active_processes;
  QList<Task> tasks;
  QUuid selected_execution_id;
};
