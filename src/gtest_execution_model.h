#ifndef GTESTEXECUTIONMODEL_H
#define GTESTEXECUTIONMODEL_H

#include <QQmlEngine>

#include "task_system.h"
#include "test_execution_model.h"

class GTestExecutionModel : public TestExecutionModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString taskName READ GetTaskName NOTIFY taskNameChanged)
 public:
  explicit GTestExecutionModel(QObject* parent = nullptr);
  QString GetTaskName() const;

 signals:
  void taskNameChanged();

 private:
  void ReloadExecution();
  void ReRunTestCase(const QString id, bool repeat_until_fail);

  TaskExecution exec;
  int current_output_line;
  int test_count;
  QString current_test_case;
};

#endif  // GTESTEXECUTIONMODEL_H
