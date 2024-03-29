#include "test_execution_model.h"

#include "theme.h"

#define LOG() qDebug() << "[TestExecutionModel]"

TestExecutionModel::TestExecutionModel(QObject* parent)
    : TextListModel(parent), test_count(-1), has_preparation_test(false) {
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
  if (test_count < 0 || GetCurrentTestCount() < test_count) {
    QString result =
        "Running (" + QString::number(GetCurrentTestCount()) + " of ";
    if (test_count < 0) {
      result += "\?\?)";
    } else {
      result += QString::number(test_count) + ')';
    }
    return result + " for " + duration + "...";
  } else if (test_count == 0) {
    return "No tests found";
  } else if (failed > 0) {
    return QString::number(failed) + " of " + QString::number(test_count) +
           " failed in " + duration;
  } else {
    return "Executed " + QString::number(GetCurrentTestCount()) + " in " +
           duration;
  }
}

float TestExecutionModel::GetProgress() const {
  if (test_count <= 0) {
    return 0;
  } else {
    return (float)GetCurrentTestCount() / test_count;
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
  FinishTestPreparationIfNecessary(true);
  LOG() << "New test started:" << test_suite << test_case << rerun_id;
  Test test;
  test.test_suite = test_suite;
  test.test_case = test_case;
  test.rerun_id = rerun_id;
  last_test_start = std::chrono::system_clock::now();
  tests.append(test);
  emit statusChanged();
}

void TestExecutionModel::AppendOutputToCurrentTest(const QString& output) {
  Q_ASSERT(!tests.isEmpty());
  Test& test = tests.last();
  LOG() << "Output added to" << test.test_suite << test.test_case;
  test.output += output;
  if (tests.size() - 1 == GetSelectedItemIndex()) {
    emit selectedTestOutputChanged();
  }
}

void TestExecutionModel::AppendTestPreparationOutput(const QString& output) {
  if (tests.isEmpty()) {
    StartTest("", "Before Tests Start", "");
    has_preparation_test = true;
  }
  AppendOutputToCurrentTest(output);
}

void TestExecutionModel::FinishCurrentTest(bool success) {
  Q_ASSERT(!tests.isEmpty());
  Test& test = tests.last();
  Q_ASSERT(test.status == TestStatus::kRunning);
  LOG() << "Test finished:" << test.test_suite << test.test_case
        << "success:" << success;
  test.status = success ? TestStatus::kCompleted : TestStatus::kFailed;
  test.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - last_test_start);
  emit statusChanged();
}

void TestExecutionModel::SetTestCount(int count) {
  if (count < 0) {
    count = GetCurrentTestCount();
  }
  FinishTestPreparationIfNecessary(count > GetCurrentTestCount());
  LOG() << "Total test count set to" << count;
  test_count = count;
  emit statusChanged();
}

bool TestExecutionModel::IsSelectedTestRerunnable() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? false : !tests[i].rerun_id.isEmpty();
}

void TestExecutionModel::Clear() {
  test_count = -1;
  has_preparation_test = false;
  tests.clear();
  selectItemByIndex(-1);
}

void TestExecutionModel::rerunSelectedTest(bool repeat_until_fail) {
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  QString id = tests[i].rerun_id;
  LOG() << "Re-running test with ID" << id
        << "repeat until fail:" << repeat_until_fail;
  emit rerunTest(id, repeat_until_fail);
}

QVariantList TestExecutionModel::GetRow(int i) const {
  static const Theme kTheme;
  const Test& t = tests[i];
  QString color;
  QString status;
  if (t.status == TestStatus::kCompleted) {
    color = kTheme.kColorGreen;
    status = "status:completed";
  } else if (t.status == TestStatus::kFailed) {
    color = "red";
    status = "status:failed";
  } else {
    color = kTheme.kColorBorder;
    status = "status:running";
  }
  return {t.test_case,
          t.test_suite,
          "fiber_manual_record",
          color,
          FormatDuration(t.duration),
          status};
}

int TestExecutionModel::GetRowCount() const { return tests.size(); }

void TestExecutionModel::FinishTestPreparationIfNecessary(bool success) {
  if (tests.size() != 1 || tests[0].status != TestStatus::kRunning) {
    return;
  }
  FinishCurrentTest(success);
}

int TestExecutionModel::GetCurrentTestCount() const {
  int result = 0;
  for (const Test& t : tests) {
    if (t.status != TestStatus::kRunning) {
      result++;
    }
  }
  if (has_preparation_test) {
    result--;
  }
  return std::max(result, 0);
}
