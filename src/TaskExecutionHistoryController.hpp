#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"
#include "TaskSystem.hpp"

class TaskExecutionListModel : public QVariantListModel {
 public:
  TaskExecutionListModel(QObject* parent, QUuid& selected_execution_id);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
  const TaskExecution* FindById(QUuid id) const;

  QList<TaskExecution> list;
  QUuid& selected_execution_id;
};

class TaskExecutionHistoryController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      bool executionsEmpty READ AreExecutionsEmpty NOTIFY executionsChanged)
  Q_PROPERTY(TaskExecutionListModel* executions MEMBER executions CONSTANT)
  Q_PROPERTY(
      QString executionName MEMBER execution_name NOTIFY executionChanged)
  Q_PROPERTY(
      QString executionStatus MEMBER execution_status NOTIFY executionChanged)
  Q_PROPERTY(
      QString executionOutput MEMBER execution_output NOTIFY executionChanged)
  Q_PROPERTY(bool executionRunning READ IsSelectedExecutionRunning NOTIFY
                 executionChanged)
 public:
  explicit TaskExecutionHistoryController(QObject* parent = nullptr);
  bool AreExecutionsEmpty() const;
  bool IsSelectedExecutionRunning() const;

 public slots:
  void SelectExecution(QUuid id);
  void HandleExecutionFinished(QUuid id);
  void HandleExecutionOutputChanged(QUuid id);
  void RemoveFinishedExecutions();

 signals:
  void executionsChanged();
  void executionChanged();

 private:
  void LoadExecutions(bool update_selected_execution);
  void LoadSelectedExecutionOutput();
  void DisplaySelectedExecution();

  QString execution_name;
  QString execution_status;
  QString execution_output;
  QUuid execution_id;
  TaskExecutionListModel* executions;
};
