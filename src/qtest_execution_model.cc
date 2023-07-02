#include "qtest_execution_model.h"

#include <QRegularExpression>

#include "application.h"

#define LOG() qDebug() << "[QTestExecutionModel]"

QTestExecutionModel::QTestExecutionModel(QObject* parent)
    : TestExecutionModel(parent), current_output_line(0), tests_seen(0) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("QTest Execution");
  connect(&app.task, &TaskSystem::executionOutputChanged, this,
          [this, &app](QUuid id) {
            if (app.task.GetSelectedExecutionId() == id) {
              ReloadExecution();
            }
          });
  connect(&app.task, &TaskSystem::executionFinished, this,
          [this, &app](QUuid id) {
            if (app.task.GetSelectedExecutionId() == id) {
              LOG() << "Tests seen:" << tests_seen;
              SetTestCount(tests_seen);
            }
          });
  connect(this, &TestExecutionModel::rerunTest, this,
          &QTestExecutionModel::ReRunTestCase);
  ReloadExecution();
}

QString QTestExecutionModel::GetTaskName() const { return exec.task_name; }

void QTestExecutionModel::ReloadExecution() {
  Application& app = Application::Get();
  QUuid id = app.task.GetSelectedExecutionId();
  app.task.FetchExecution(id, true).Then(
      this, [this](const TaskExecution& exec) {
        static const QRegularExpression kSuiteStartRegex(
            "\\*+ Start testing of (.+) \\*+");
        this->exec = exec;
        emit taskNameChanged();
        QStringList lines = exec.output.split('\n', Qt::SkipEmptyParts);
        for (; current_output_line < lines.size(); current_output_line++) {
          const QString& line = lines[current_output_line];
          LOG() << "New raw line:" << line;
          if (line.startsWith("Config: Using QtTest library") ||
              line.startsWith("Totals: ") ||
              line.startsWith("********* Finished")) {
            continue;
          }
          QRegularExpressionMatch m = kSuiteStartRegex.match(line);
          if (m.hasMatch()) {
            current_test_suite = m.captured(1);
            LOG() << "Test suite started:" << current_test_suite;
            continue;
          }
          bool pass = line.startsWith("PASS");
          bool fail = line.startsWith("FAIL");
          if (pass || fail) {
            StartCurrentTestCaseIfNecessary(line);
            FinishCurrentTest(pass);
            tests_seen++;
            LOG() << "Test" << current_test_case << "passed:" << pass;
            if (pass) {
              // In case of fail - the line will have output on it
              continue;
            }
          }
          if (current_test_suite.isEmpty()) {
            continue;
          }
          QString test_id_start = current_test_suite + "::";
          int i = line.indexOf(test_id_start);
          if (i < 0) {
            if (!current_test_case.isEmpty()) {
              AppendOutputToCurrentTest(line + '\n');
            }
          } else {
            StartCurrentTestCaseIfNecessary(line);
            QString test_id = test_id_start + current_test_case + "() ";
            QString output = line.sliced(i + test_id.size());
            AppendOutputToCurrentTest(output + '\n');
          }
        }
        if (exec.exit_code) {
          LOG() << "Tests seen:" << tests_seen;
          SetTestCount(tests_seen);
        }
      });
}

void QTestExecutionModel::StartCurrentTestCaseIfNecessary(const QString& line) {
  QString prefix = ": " + current_test_suite + "::";
  int start = line.indexOf(prefix) + prefix.size();
  int end = line.indexOf("()", start);
  QString test_case = line.sliced(start, end - start);
  if (current_test_case != test_case) {
    current_test_case = test_case;
    LOG() << "Test" << current_test_case << "started";
    QString rerun_id;
    if (!current_test_case.startsWith("initTestCase") &&
        current_test_case != "cleanupTestCase" && current_test_case != "init" &&
        current_test_case != "cleanup") {
      rerun_id = current_test_case;
    }
    StartTest(current_test_suite, current_test_case, rerun_id);
  }
}

void QTestExecutionModel::ReRunTestCase(const QString id,
                                        bool repeat_until_fail) {
  Application::Get().task.RunTaskOfExecution(exec, repeat_until_fail,
                                             "QtestExecution.qml", {id});
}
