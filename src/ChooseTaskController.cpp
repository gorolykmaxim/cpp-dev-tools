#include "ChooseTaskController.hpp"

#include "Application.hpp"
#include "QVariantListModel.hpp"

ChooseTaskController::ChooseTaskController(QObject *parent)
    : QObject(parent),
      is_loading(true),
      tasks(
          new SimpleQVariantListModel(this, {{0, "idx"}, {1, "title"}}, {1})) {}

bool ChooseTaskController::ShouldShowPlaceholder() const {
  return is_loading || tasks->list.isEmpty();
}

bool ChooseTaskController::IsLoading() const { return is_loading; }

static bool CompareExecs(const QString &a, const QString &b) {
  if (a.size() != b.size()) {
    return a.size() < b.size();
  } else {
    return a < b;
  }
}

static QString ShortenTaskCmd(QString cmd, const Project &project) {
  if (cmd.startsWith(project.path)) {
    cmd.replace(project.path, ".");
  }
  return cmd;
}

void ChooseTaskController::FindTasks() {
  SetIsLoading(true);
  QString path = project.path;
  Application::Get().RunIOTask<QList<QString>>(
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
      [this](QList<QString> execs) {
        tasks->list.clear();
        for (int i = 0; i < execs.size(); i++) {
          tasks->list.append({i, ShortenTaskCmd(execs[i], project)});
        }
        tasks->Load();
        SetIsLoading(false);
      });
}

void ChooseTaskController::SetIsLoading(bool value) {
  is_loading = value;
  emit isLoadingChanged();
}
