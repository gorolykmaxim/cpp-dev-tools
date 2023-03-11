#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"
#include "TaskExecutor.hpp"

class TaskExecutionListModel : public QVariantListModel {
 public:
  explicit TaskExecutionListModel(QObject* parent);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
};

class TaskExecutionHistoryController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      bool executionsEmpty READ AreExecutionsEmpty NOTIFY executionsChanged)
  Q_PROPERTY(TaskExecutionListModel* executions MEMBER executions CONSTANT)
  Q_PROPERTY(
      QString executionCommand MEMBER execution_command NOTIFY executionChanged)
  Q_PROPERTY(
      QString executionStatus MEMBER execution_status NOTIFY executionChanged)
  Q_PROPERTY(
      QString executionOutput MEMBER execution_output NOTIFY executionChanged)
 public:
  explicit TaskExecutionHistoryController(QObject* parent = nullptr);
  bool AreExecutionsEmpty() const;

  TaskExecutionListModel* executions;
  QString execution_command;
  QString execution_status;
  QString execution_output;
  QUuid execution_id;

 public slots:
  void SelectExecution(QUuid id);
  void HandleExecutionFinished(QUuid id);
  void HandleExecutionOutputChanged(QUuid id);

 signals:
  void executionsChanged();
  void executionChanged();
};
