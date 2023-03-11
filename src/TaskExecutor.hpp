#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QProcess>
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
};

class TaskExecutor : public QObject {
  Q_OBJECT
 public:
  void ExecuteTask(const QString& command);
  void FetchExecutions(
      QObject* requestor, QUuid project_id,
      const std::function<void(const QList<TaskExecution>&)>& callback) const;
  void FetchExecutionOutput(
      QObject* requestor, QUuid execution_id,
      const std::function<void(const QString&)>& callback) const;

 signals:
  void executionOutputChanged(QUuid exec_id);
  void executionFinished(QUuid exec_id);

 private:
  void AppendToExecutionOutput(QUuid id, const QByteArray& data);
  void FinishExecution(QUuid id, QProcess* process);

  QHash<QUuid, TaskExecution> active_executions;
  QHash<QUuid, QString> active_execution_outputs;
};
