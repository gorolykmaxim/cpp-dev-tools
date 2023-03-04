#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QUuid>
#include <optional>

struct TaskExecution {
  QUuid id;
  QDateTime start_time;
  QString command;
  QString output;
  std::optional<int> exit_code;
};

class TaskExecutor : public QObject {
  Q_OBJECT
 public:
  void ExecuteTask(const QString& command);
  TaskExecution* FindExecutionById(QUuid id);

 signals:
  void executionOutputChanged(QUuid exec_id);
  void executionFinished(QUuid exec_id);

 private:
  void AppendToExecutionOutput(QUuid id, const QByteArray& data);
  void SetExecutionExitCode(QUuid id, int code);

  QList<TaskExecution> executions;
};
