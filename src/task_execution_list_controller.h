#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "qvariant_list_model.h"
#include "task_system.h"

class TaskExecutionListModel : public QVariantListModel {
 public:
  TaskExecutionListModel(QObject* parent);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;

  QList<TaskExecution> list;
};

class TaskExecutionListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      bool executionsEmpty READ AreExecutionsEmpty NOTIFY executionsChanged)
  Q_PROPERTY(TaskExecutionListModel* executions MEMBER executions CONSTANT)
  Q_PROPERTY(bool executionRunning READ IsSelectedExecutionRunning NOTIFY
                 executionChanged)
  Q_PROPERTY(
      int executionTaskIndex READ IndexOfExecutionTask NOTIFY executionChanged)
 public:
  explicit TaskExecutionListController(QObject* parent = nullptr);
  bool AreExecutionsEmpty() const;
  bool IsSelectedExecutionRunning() const;
  int IndexOfExecutionTask() const;

 public slots:
  void removeFinishedExecutions();
  void openExecutionOutput() const;

 signals:
  void executionsChanged();
  void executionChanged();

 private:
  void LoadExecutions();

  TaskExecutionListModel* executions;
};
