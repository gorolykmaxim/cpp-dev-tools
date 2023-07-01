#ifndef TESTEXECUTIONMODEL_H
#define TESTEXECUTIONMODEL_H

#include <QQmlEngine>
#include <chrono>

#include "text_list_model.h"

enum class TestStatus {
  kRunning,
  kCompleted,
  kFailed,
};

struct Test {
  QString test_case;
  QString test_suite;
  QString output;
  TestStatus status = TestStatus::kRunning;
  std::chrono::milliseconds duration = std::chrono::milliseconds(0);
};

class TestExecutionModel : public TextListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString selectedTestOutput READ GetSelectedTestOutput NOTIFY
                 selectedTestOutputChanged)
  Q_PROPERTY(QString status READ GetStatus NOTIFY statusChanged)
  Q_PROPERTY(float progress READ GetProgress NOTIFY statusChanged)
  Q_PROPERTY(
      QString progressBarColor READ GetProgressBarColor NOTIFY statusChanged)
 public:
  explicit TestExecutionModel(QObject* parent = nullptr);
  QString GetSelectedTestOutput() const;
  QString GetStatus() const;
  float GetProgress() const;
  QString GetProgressBarColor() const;
  void StartTest(const QString& test_suite, const QString& test_case);
  void AppendOutputToCurrentTest(const QString& output);
  void FinishCurrentTest(bool success);
  void SetTestCount(int count);

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void selectedTestOutputChanged();
  void statusChanged();

 private:
  std::chrono::system_clock::time_point last_test_start;
  QList<Test> tests;
  int test_count;
};

#endif  // TESTEXECUTIONMODEL_H
