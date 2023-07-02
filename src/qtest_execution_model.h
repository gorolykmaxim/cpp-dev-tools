#ifndef QTESTEXECUTIONMODEL_H
#define QTESTEXECUTIONMODEL_H

#include <QQmlEngine>

#include "task_system.h"
#include "test_execution_model.h"

class QTestExecutionModel : public TestExecutionModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString taskName READ GetTaskName NOTIFY taskNameChanged)
 public:
  explicit QTestExecutionModel(QObject* parent = nullptr);
  QString GetTaskName() const;

 signals:
  void taskNameChanged();

 private:
  void ReloadExecution();
  void StartCurrentTestCaseIfNecessary(const QString& line);
  void ReRunTestCase(const QString id, bool repeat_until_fail);

  TaskExecution exec;
  QString current_test_suite;
  QString current_test_case;
  int current_output_line;
  int tests_seen;
};

#endif  // QTESTEXECUTIONMODEL_H
