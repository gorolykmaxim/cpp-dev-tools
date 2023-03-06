#include "ChooseTaskController.hpp"

#include "Application.hpp"
#include "Project.hpp"
#include "QVariantListModel.hpp"

#define LOG() qDebug() << "[ChooseTaskController]"

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

ChooseTaskController::ChooseTaskController(QObject *parent)
    : QObject(parent),
      tasks(new SimpleQVariantListModel(this, {{0, "idx"}, {1, "title"}}, {1})),
      is_loading(true) {
  Application &app = Application::Get();
  app.view_controller.SetWindowTitle("Run Task");
  SetIsLoading(true);
  Project project = app.project_context.GetCurrentProject();
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
        for (int i = 0; i < execs.size(); i++) {
          tasks->list.append({i, ShortenTaskCmd(execs[i], project)});
        }
        tasks->Load();
        SetIsLoading(false);
      });
}

bool ChooseTaskController::ShouldShowPlaceholder() const {
  return is_loading || tasks->list.isEmpty();
}

bool ChooseTaskController::IsLoading() const { return is_loading; }

void ChooseTaskController::ExecTask(int i) {
  const QString cmd = tasks->list[i][1].toString();
  Application::Get().task_executor.ExecuteTask(cmd);
}

void ChooseTaskController::SetIsLoading(bool value) {
  is_loading = value;
  emit isLoadingChanged();
}
