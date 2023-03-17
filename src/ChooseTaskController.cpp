#include "ChooseTaskController.hpp"

#include "Application.hpp"

#define LOG() qDebug() << "[ChooseTaskController]"

static bool CompareExecs(const QString &a, const QString &b) {
  if (a.size() != b.size()) {
    return a.size() < b.size();
  } else {
    return a < b;
  }
}

ChooseTaskController::ChooseTaskController(QObject *parent)
    : QObject(parent),
      tasks(new SimpleQVariantListModel(this, {{0, "title"}, {1, "command"}},
                                        {0})),
      is_loading(true) {
  Application &app = Application::Get();
  SetIsLoading(true);
  QString project_path = app.project.GetCurrentProject().path;
  app.RunIOTask<QList<QString>>(
      this,
      [project_path] {
        QList<QString> results;
        QDirIterator it(project_path, QDir::Files | QDir::Executable,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
          QString exec_path = it.next();
          exec_path.replace(project_path, ".");
          results.append(exec_path);
        }
        std::sort(results.begin(), results.end(), CompareExecs);
        return results;
      },
      [this](QList<QString> execs) {
        tasks->list.clear();
        for (const QString &exec : execs) {
          tasks->list.append({exec, exec});
        }
        tasks->Load();
        SetIsLoading(false);
      });
}

bool ChooseTaskController::ShouldShowPlaceholder() const {
  return is_loading || tasks->list.isEmpty();
}

bool ChooseTaskController::IsLoading() const { return is_loading; }

void ChooseTaskController::SetIsLoading(bool value) {
  is_loading = value;
  emit isLoadingChanged();
}
