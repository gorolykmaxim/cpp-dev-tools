#include "ChooseTaskController.hpp"

#include "Application.hpp"
#include "Project.hpp"

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
  const Project &project = app.project.GetCurrentProject();
  QString path = project.path;
  app.RunIOTask<QList<QString>>(
      this,
      [path] {
        QList<QString> results;
        QDirIterator it(path, QDir::Files | QDir::Executable,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
          results.append(it.next());
        }
        std::sort(results.begin(), results.end(), CompareExecs);
        return results;
      },
      [this, project](QList<QString> execs) {
        tasks->list.clear();
        for (const QString &exec : execs) {
          QString short_cmd = TaskExecution::ShortenCommand(exec, project);
          tasks->list.append({short_cmd, exec});
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
