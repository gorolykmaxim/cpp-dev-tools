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

#include "Project.hpp"

struct TaskExecution {
  QUuid id;
  QDateTime start_time;
  QString command;
  std::optional<int> exit_code;

  bool operator==(const TaskExecution& another) const;
  bool operator!=(const TaskExecution& another) const;
};

struct TaskExecutionOutput {
  QSet<int> stderr_line_indices;
  QString output;
};

class TaskSystem : public QObject {
  Q_OBJECT
 public:
  static QString ShortenCommand(QString cmd, const Project& project);
  void ExecuteTask(const QString& command);
  void KillAllTasks();
  void FetchExecutions(
      QObject* requestor, QUuid project_id,
      const std::function<void(const QList<TaskExecution>&)>& callback) const;
  void FetchExecutionOutput(
      QObject* requestor, QUuid execution_id,
      const std::function<void(const TaskExecutionOutput&)>& callback) const;

 signals:
  void executionOutputChanged(QUuid exec_id);
  void executionFinished(QUuid exec_id);

 private:
  static TaskExecution ReadExecutionFromSql(QSqlQuery& query);
  static TaskExecutionOutput ReadExecutionOutputFromSql(QSqlQuery& query);
  void AppendToExecutionOutput(QUuid id, const QByteArray& data,
                               bool is_stderr);
  void FinishExecution(QUuid id, QProcess* process);

  QHash<QUuid, TaskExecution> active_executions;
  QHash<QUuid, TaskExecutionOutput> active_execution_outputs;
  QHash<QUuid, QProcess*> active_processes;
};
