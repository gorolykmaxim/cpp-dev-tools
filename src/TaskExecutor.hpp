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
  // TODO: from UI perspective we don't need to keep output as a part of this
  // struct, because the struct is need to display in a list, but output
  // is only needed when a struct from the list is selected. We should not fetch
  // output from the database when we try to display the list of executions.
  // Thus the output should be stored separately from TaskExecution.
  QString output;
  std::optional<int> exit_code;

  static QString ShortenTaskCmd(QString cmd, const Project& project);
};

class TaskExecutor : public QObject {
  Q_OBJECT
 public:
  void ExecuteTask(const QString& command);
  TaskExecution* FindExecutionById(QUuid id);
  // TODO: this won't work because we would need to store executions in database
  // load them from there.
  const QList<TaskExecution>& GetExecutions() const;

 signals:
  void executionOutputChanged(QUuid exec_id);
  void executionFinished(QUuid exec_id);

 private:
  void AppendToExecutionOutput(QUuid id, const QByteArray& data);
  void FinishExecution(QUuid id, QProcess* process);

  // TODO: this can't contain all executions, because TaskExecutor lives long
  // enough to outlive a project, which means this list will contain executions
  // of a different project. In reality it should only contain active executions
  // and when a project changes - these active executions should be terminated.
  QList<TaskExecution> executions;
};
