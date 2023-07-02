#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "task_system.h"
#include "text_list_model.h"

class TaskExecutionListModel : public TextListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(bool executionRunning READ IsSelectedExecutionRunning NOTIFY
                 executionChanged)
 public:
  TaskExecutionListModel(QObject* parent = nullptr);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
  void Reload();
  bool IsSelectedExecutionRunning() const;

  QList<TaskExecution> list;

 public slots:
  void rerunSelectedExecution(bool repeat_until_fail, const QString& view);
  void removeFinishedExecutions();
  void openExecutionOutput() const;

 signals:
  void executionChanged();
};
