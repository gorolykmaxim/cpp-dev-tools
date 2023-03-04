#include "ChooseTaskController.hpp"

#include "Application.hpp"
#include "Process.hpp"
#include "Project.hpp"
#include "QVariantListModel.hpp"

#define LOG() qDebug() << "[ChooseTaskController]"

ChooseTaskController::ChooseTaskController(QObject *parent)
    : QObject(parent),
      tasks(new SimpleQVariantListModel(this, {{0, "idx"}, {1, "title"}}, {1})),
      is_loading(true) {}

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
  Application &app = Application::Get();
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

void ChooseTaskController::ExecTask(int i) {
  const QString cmd = tasks->list[i][1].toString();
  LOG() << "Executing task" << cmd;
  Process *process = new Process(cmd, this);
  QObject::connect(process, &Process::outputChanged, this,
                   [] { LOG() << "One of the processes printed output"; });
  QObject::connect(process, &Process::finished, this, [process] {
    LOG() << process->GetOutput();
    LOG() << process->GetCommand() << "finished with" << process->GetExitCode();
  });
  process->Start();
}

void ChooseTaskController::SetIsLoading(bool value) {
  is_loading = value;
  emit isLoadingChanged();
}
