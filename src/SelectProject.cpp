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
        UIDataField{"vFilter", ""},
      },
      {
        UIListField{"vProjects", role_names, projects},
      },
      {"vaFilterChanged", "vaProjectSelected", "vaNewProject"});
  EXEC_NEXT(KeepAlive);
}
