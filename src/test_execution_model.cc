#include "test_execution_model.h"

#include "application.h"
#include "theme.h"

TestExecutionModel::TestExecutionModel(QObject* parent)
    : TextListModel(parent), test_count(-1) {
  SetRoleNames({{0, "title"},
                {1, "subTitle"},
                {2, "icon"},
                {3, "iconColor"},
                {4, "rightText"},
                {5, "testStatus"}});
  searchable_roles = {0, 1, 5};
  SetEmptyListPlaceholder("No tests executed yet");
  connect(this, &TextListModel::selectedItemChanged, this,
          [this] { emit selectedTestOutputChanged(); });
  Application::Get().view.SetWindowTitle("Tests");
}

QString TestExecutionModel::GetSelectedTestOutput() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? "" : tests[i].output;
}

static QString FormatDuration(std::chrono::milliseconds ms) {
  QString duration = QString::number(ms.count()) + "ms";
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
  if (sec.count() > 0) {
    duration = QString::number(sec.count()) + "s";
    auto min = std::chrono::duration_cast<std::chrono::minutes>(ms);
    if (min.count() > 0) {
      duration = QString::number(min.count()) + " min";
    }
  }
  return duration;
}

QString TestExecutionModel::GetStatus() const {
  std::chrono::milliseconds total(0);
  int failed = 0;
  for (const Test& test : tests) {
    total += test.duration;
    if (test.status == TestStatus::kFailed) {
      failed++;
    }
  }
  QString duration = FormatDuration(total);
  if (test_count < 0 || tests.size() < test_count) {
    QString result = "Running (" + QString::number(tests.size()) + " of ";
    if (test_count < 0) {
      result += "\?\?)";
    } else {
      result += QString::number(test_count) + ')';
    }
    return result + " for " + duration + "...";
  } else if (failed > 0) {
    return QString::number(failed) + " of " + QString::number(test_count) +
           " failed in " + duration;
  } else {
    return "Executed " + QString::number(tests.size()) + " in " + duration;
  }
}

float TestExecutionModel::GetProgress() const {
  if (test_count <= 0) {
    return 0;
  } else {
    return (float)tests.size() / test_count;
  }
}

QString TestExecutionModel::GetProgressBarColor() const {
  static const Theme kTheme;
  bool success = true;
  for (const Test& test : tests) {
    if (test.status == TestStatus::kFailed) {
      success = false;
      break;
    }
  }
  return success ? kTheme.kColorGreen : "red";
}

void TestExecutionModel::StartTest(const QString& test_suite,
                                   const QString& test_case,
                                   const QString& rerun_id) {
  Test test;
  test.test_suite = test_suite;
  test.test_case = test_case;
  test.rerun_id = rerun_id;
  last_test_start = std::chrono::system_clock::now();
  tests.append(test);
  emit statusChanged();
  Load(tests.size() - 1);
}

void TestExecutionModel::AppendOutputToCurrentTest(const QString& output) {
  Q_ASSERT(!tests.isEmpty());
  Test& test = tests.last();
  Q_ASSERT(test.status == TestStatus::kRunning);
  test.output += output;
  if (tests.size() - 1 == GetSelectedItemIndex()) {
    emit selectedTestOutputChanged();
  }
}

void TestExecutionModel::FinishCurrentTest(bool success) {
  Q_ASSERT(!tests.isEmpty());
  Test& test = tests.last();
  Q_ASSERT(test.status == TestStatus::kRunning);
  test.status = success ? TestStatus::kCompleted : TestStatus::kFailed;
  test.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - last_test_start);
  emit statusChanged();
  Load(tests.size() - 1);
}

void TestExecutionModel::SetTestCount(int count) {
  test_count = count;
  emit statusChanged();
}

void TestExecutionModel::rerunSelectedTest(bool repeat_until_fail) {
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  emit rerunTest(tests[i].rerun_id, repeat_until_fail);
}

QVariantList TestExecutionModel::GetRow(int i) const {
  static const Theme kTheme;
  const Test& t = tests[i];
  QString color;
  QString status;
  if (t.status == TestStatus::kCompleted) {
    color = kTheme.kColorGreen;
    status = "completed";
  } else if (t.status == TestStatus::kFailed) {
    color = "red";
    status = "failed";
  } else {
    color = kTheme.kColorBorder;
    status = "running";
  }
  return {t.test_case,
          t.test_suite,
          "fiber_manual_record",
          color,
          FormatDuration(t.duration),
          status};
}

int TestExecutionModel::GetRowCount() const { return tests.size(); }
