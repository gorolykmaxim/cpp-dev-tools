#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QSqlQuery>
#include <QString>
#include <QUuid>
#include <optional>

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

  bool operator==(const TaskExecution& another) const;
  bool operator!=(const TaskExecution& another) const;
};

struct TaskExecutionOutput {
  QSet<int> stderr_line_indices;
  QString output;
};

class RunTaskCommand : public QObject {
  Q_OBJECT
 public:
  virtual void Start() = 0;
  virtual void Cancel(bool forcefully) = 0;

 signals:
  void outputWritten(const QString& data, bool is_stderr);
  void finished(int exit_code);
};

class TaskSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      QString currentTaskName READ GetCurrentTaskName NOTIFY taskListRefreshed)
 public:
  static QString GetName(const Task& task);
  void KillAllTasks();
  void FetchExecutions(
      QObject* requestor, QUuid project_id,
      const std::function<void(const QList<TaskExecution>&)>& callback) const;
  void FetchExecutionOutput(
      QObject* requestor, QUuid execution_id,
      const std::function<void(const TaskExecutionOutput&)>& callback) const;
  bool IsExecutionRunning(QUuid execution_id) const;
  void FindTasks();
  const QList<Task>& GetTasks() const;
  QString GetCurrentTaskName() const;

 public slots:
  void ExecuteTask(int i, bool repeat_until_fail = false);
  void CancelExecution(QUuid execution_id, bool forcefully);

 signals:
  void executionOutputChanged(QUuid exec_id);
  void executionFinished(QUuid exec_id);
  void taskListRefreshed();

 private:
  static TaskExecution ReadExecutionFromSql(QSqlQuery& query);
  static TaskExecutionOutput ReadExecutionOutputFromSql(QSqlQuery& query);
  void AppendToExecutionOutput(QUuid id, QString data, bool is_stderr);
  void FinishExecution(QUuid id, int exit_code);

  QHash<QUuid, TaskExecution> active_executions;
  QHash<QUuid, TaskExecutionOutput> active_outputs;
  QHash<QUuid, QSharedPointer<RunTaskCommand>> active_commands;
  QList<Task> tasks;
};
