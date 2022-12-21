#include "SelectProject.hpp"
#include "UI.hpp"
#include "Process.hpp"
#include "SaveUserConfig.hpp"
#include "Threads.hpp"

SelectProject::SelectProject() {
  EXEC_NEXT(SanitizeProjectList);
}

void SelectProject::SanitizeProjectList(AppData& app) {
  qDebug() << "Removing projects that no longer exist";
  QList<Project> projects = app.projects;
  QPtr<SelectProject> self = ProcessSharedPtr(app, this);
  ScheduleIOTask<QList<Project>>(app, [projects] () {
    QList<Project> result;
    for (const Project& project: projects) {
      if (QFile(project.path).exists()) {
        result.append(project);
      } else {
        qDebug() << "Project" << project.path << "no longer exists - removing";
      }
    }
    return result;
  }, [&app, self] (QList<Project> projects) {
    app.projects = projects;
    ScheduleProcess<SaveUserConfig>(app, nullptr);
    WakeUpAndExecuteProcess(app, *self);
  });
  EXEC_NEXT(LoadLastProjectOrDisplaySelectProjectView);
}

void SelectProject::LoadLastProjectOrDisplaySelectProjectView(AppData& app) {
  if (!app.projects.isEmpty() &&
      app.projects[0].path == app.current_project_path) {
    qDebug() << "Loading current project" << app.current_project_path;
    load_project_file = ScheduleProcess<LoadTaskConfig>(
        app, this, app.current_project_path);
    EXEC_NEXT(HandleOpenLastProjectCompletion);
  } else {
    app.current_project_path = "";
    DisplaySelectProjectView(app);
  }
}

void SelectProject::HandleOpenLastProjectCompletion(AppData& app) {
  if (!load_project_file->success) {
    DisplaySelectProjectView(app);
  }
}

void SelectProject::DisplaySelectProjectView(AppData& app) {
  QHash<int, QByteArray> role_names = {
    {0, "idx"},
    {1, "title"},
    {2, "subTitle"},
  };
  DisplayView(
      app,
      kViewSlot,
      "SelectProjectView.qml",
      {
        UIDataField{"windowTitle", "Open Project"},
        UIDataField{"vFilter", ""},
      },
      {
        UIListField{"vProjects", role_names, MakeFilteredListOfProjects(app)},
      });
  DisplayMenuBar(app);
  DisplayStatusBar(app);
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaNewProject", *this,
                         EXEC(this, OpenNewProject));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaProjectSelected", *this,
                         EXEC(this, OpenExistingProject));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaFilterChanged", *this,
                         EXEC(this, FilterProjects));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaRemoveProject", *this,
                         EXEC(this, RemoveProject));
}

void SelectProject::OpenNewProject(AppData& app) {
  open_project = ScheduleProcess<OpenProject>(app, this);
  EXEC_NEXT(HandleOpenNewProjectCompletion);
}

void SelectProject::HandleOpenNewProjectCompletion(AppData& app) {
  if (!open_project->opened) {
    DisplaySelectProjectView(app);
  }
}

void SelectProject::OpenExistingProject(AppData& app) {
  int i = GetEventArg(app, 0).toInt();
  Project& project = app.projects[i];
  load_project_file = ScheduleProcess<LoadTaskConfig>(app, this, project.path);
  EXEC_AND_WAIT_FOR_NEXT(HandleOpenExistingProjectCompletion);
}

void SelectProject::HandleOpenExistingProjectCompletion(AppData&) {
  if (!load_project_file->success) {
    EXEC_NEXT(KeepAlive);
  }
}

void SelectProject::FilterProjects(AppData& app) {
  filter = GetEventArg(app, 0).toString();
  QList<QVariantList> projects = MakeFilteredListOfProjects(app);
  GetUIListField(app, kViewSlot, "vProjects").SetItems(projects);
  EXEC_NEXT(KeepAlive);
}

void SelectProject::RemoveProject(AppData& app) {
  int i = GetEventArg(app, 0).toInt();
  app.projects.remove(i);
  ScheduleProcess<SaveUserConfig>(app, nullptr);
  QList<QVariantList> projects = MakeFilteredListOfProjects(app);
  GetUIListField(app, kViewSlot, "vProjects").SetItems(projects);
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> SelectProject::MakeFilteredListOfProjects(AppData& app) {
  QList<QVariantList> projects;
  for (int i = 0; i < app.projects.size(); i++) {
    Project& project = app.projects[i];
    QString name = project.GetFolderName();
    QString path = project.GetPathRelativeToHome();
    bool name_matches = HighlighSubstring(name, filter);
    bool path_matches = HighlighSubstring(path, filter);
    if (filter.isEmpty() || name_matches || path_matches) {
      projects.append({i, name, path});
    }
  }
  return projects;
}
