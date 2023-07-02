#include "gtest_execution_model.h"

#include <QRegularExpression>

#include "application.h"

#define LOG() qDebug() << "[GTestExecutionModel]"

GTestExecutionModel::GTestExecutionModel(QObject* parent)
    : TestExecutionModel(parent), current_output_line(0), test_count(-1) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Google Test Execution");
  connect(&app.task, &TaskSystem::executionOutputChanged, this,
          [this, &app](QUuid id) {
            if (app.task.GetSelectedExecutionId() == id) {
              ReloadExecution();
            }
          });
  connect(&app.task, &TaskSystem::executionFinished, this,
          [this, &app](QUuid id) {
            if (app.task.GetSelectedExecutionId() == id && test_count < 0) {
              SetTestCount(0);
            }
          });
  connect(this, &TestExecutionModel::rerunTest, this,
          &GTestExecutionModel::ReRunTestCase);
  ReloadExecution();
}

QString GTestExecutionModel::GetTaskName() const { return exec.task_name; }

void GTestExecutionModel::ReloadExecution() {
  Application& app = Application::Get();
  QUuid id = app.task.GetSelectedExecutionId();
  app.task.FetchExecution(id, true).Then(
      this, [this](const TaskExecution& exec) {
        static const QRegularExpression kGlobalStartRegex(
            "\\[\\=+\\] Running ([0-9]+)");
        static const QRegularExpression kTestStartRegex(
            "\\[\\s+RUN\\s+\\] (.+)\\.(.+)");
        static const QRegularExpression kTestOkRegex(
            "\\[\\s+OK\\s+\\] (.+)\\.(.+)");
        static const QRegularExpression kTestFailedRegex(
            "\\[\\s+FAILED\\s+\\] (.+)\\.(.+)");
        this->exec = exec;
        emit taskNameChanged();
        QStringList lines = exec.output.split('\n', Qt::SkipEmptyParts);
        for (; current_output_line < lines.size(); current_output_line++) {
          const QString& line = lines[current_output_line];
          LOG() << "New raw line:" << line;
          if (test_count < 0) {
            QRegularExpressionMatch m = kGlobalStartRegex.match(line);
            if (m.hasMatch()) {
              test_count = m.captured(1).toInt();
              SetTestCount(test_count);
            } else {
              AppendTestPreparationOutput(line + '\n');
            }
            continue;
          }
          if (current_test_case.isEmpty()) {
            QRegularExpressionMatch m = kTestStartRegex.match(line);
            if (m.hasMatch()) {
              QString test_suite = m.captured(1);
              current_test_case = m.captured(2);
              StartTest(test_suite, current_test_case,
                        test_suite + '.' + current_test_case);
            }
            continue;
          }
          QRegularExpressionMatch m = kTestOkRegex.match(line);
          if (m.hasMatch()) {
            FinishCurrentTest(true);
            current_test_case.clear();
            continue;
          }
          m = kTestFailedRegex.match(line);
          if (m.hasMatch()) {
            FinishCurrentTest(false);
            current_test_case.clear();
            continue;
          }
          AppendOutputToCurrentTest(line + '\n');
        }
        if (test_count < 0 && exec.exit_code) {
          SetTestCount(0);
        }
      });
}

void GTestExecutionModel::ReRunTestCase(const QString id,
                                        bool repeat_until_fail) {
  Application::Get().task.RunTaskOfExecution(
      exec, repeat_until_fail, "GtestExecution.qml", {"--gtest_filter=" + id});
}
