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

  static QString ShortenCommand(QString cmd, const Project& project);
  static TaskExecution ReadFromSql(QSqlQuery& query);
  bool operator==(const TaskExecution& another) const;
  bool operator!=(const TaskExecution& another) const;
};

struct TaskExecutionOutput {
  QSet<int> stderr_line_indices;
  QString output;

  static TaskExecutionOutput ReadFromSql(QSqlQuery& query);
};

class TaskSystem : public QObject {
  Q_OBJECT
 public:
  void ExecuteTask(const QString& command);
  void KillAllTasks();
  void FetchExecutions(
      QObject* requestor, QUuid project_id,
      const std::function<void(const QList<TaskExecution>&)>& callback) const;
  void FetchExecutionOutput(
      QObject* requestor, QUuid execution_id,
      const std::function<void(const TaskExecutionOutput&)>& callback) const;
  void ClearExecutionHistory();

 signals:
  void executionOutputChanged(QUuid exec_id);
  void executionFinished(QUuid exec_id);

 private:
  void AppendToExecutionOutput(QUuid id, const QByteArray& data,
                               bool is_stderr);
  void FinishExecution(QUuid id, QProcess* process);

  QHash<QUuid, TaskExecution> active_executions;
  QHash<QUuid, TaskExecutionOutput> active_execution_outputs;
  QHash<QUuid, QProcess*> active_processes;
};
