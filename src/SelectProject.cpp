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
  QString home_str = QStandardPaths::writableLocation(
      QStandardPaths::HomeLocation);
  QList<QVariantList> projects;
  for (int i = 0; i < app.projects.size(); i++) {
    Project& project = app.projects[i];
    projects.append({i,
                     project.GetFolderName(),
                     project.GetPathRelativeToHome()});
  }
  DisplayView(
      app,
      kViewSlot,
      "SelectProjectView.qml",
      {
        UIDataField{kWindowTitle, "Open Project"},
        UIDataField{"vFilter", ""},
      },
      {
        UIListField{"vProjects", role_names, projects},
      });
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaNewProject", *this,
                         EXEC(this, OpenNewProject));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaProjectSelected", *this,
                         EXEC(this, OpenExistingProject));
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
