#include "SelectProject.hpp"
#include "UI.hpp"
#include "Process.hpp"

SelectProject::SelectProject() {
  EXEC_NEXT(DisplaySelectProjectView);
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
        UIDataField{kWindowTitle, "Open Project"},
        UIDataField{"vFilter", ""},
      },
      {
        UIListField{"vProjects", role_names, MakeFilteredListOfProjects(app)},
      });
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaNewProject", *this,
                         EXEC(this, OpenNewProject));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaProjectSelected", *this,
                         EXEC(this, OpenExistingProject));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaFilterChanged", *this,
                         EXEC(this, FilterProjects));
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

static bool HighlighSubstring(QString& str, const QString& sub_str) {
  if (sub_str.isEmpty()) {
    return false;
  }
  int i = str.toLower().indexOf(sub_str.toLower());
  if (i >= 0) {
    str.insert(i + sub_str.size(), "</b>");
    str.insert(i, "<b>");
    return true;
  } else {
    return false;
  }
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