#include "SelectProject.hpp"

#include "Db.hpp"
#include "DisplayTaskList.hpp"
#include "Process.hpp"
#include "Threads.hpp"
#include "UI.hpp"

#define LOG() qDebug() << "[SelectProject]"

static const QString kSqlDeleteProject = "DELETE FROM project WHERE id=?";

SelectProject::SelectProject() { EXEC_NEXT(SanitizeProjectList); }

void SelectProject::SanitizeProjectList(AppData& app) {
  LOG() << "Removing projects that no longer exist";
  QPtr<SelectProject> self = ProcessSharedPtr(app, this);
  ScheduleIOTask<QList<Project>>(
      app,
      [&app]() {
        DbTransaction t(app.db);
        QList<Project> projects = ExecDbQueryAndRead<Project>(
            app.db, "SELECT * FROM project ORDER BY last_open_time DESC",
            ReadProjectFromSql);
        QList<Project> filtered;
        for (const Project& project : projects) {
          if (QFile(project.path).exists()) {
            filtered.append(project);
            continue;
          }
          LOG() << "Project" << project.path << "no longer exists - removing";
          ExecDbCmd(app.db, kSqlDeleteProject, {project.id});
        }
        return filtered;
      },
      [&app, self](QList<Project> result) {
        self->projects = result;
        WakeUpAndExecuteProcess(app, *self);
      });
  EXEC_NEXT(LoadLastProjectOrDisplaySelectProjectView);
}

static void OpenSpecifiedProject(AppData& app, Project project) {
  project.is_opened = true;
  project.last_open_time = QDateTime::currentDateTime();
  app.current_project = QPtr<Project>::create(project);
  QDir::setCurrent(project.path);
  DisplayMenuBar(app);
  DisplayStatusBar(app);
  ScheduleProcess<DisplayTaskList>(app, kViewSlot);
  ExecDbCmdOnIOThread(
      app, "UPDATE project SET is_opened=?, last_open_time=? WHERE id=?",
      {project.is_opened, project.last_open_time, project.id});
}

void SelectProject::LoadLastProjectOrDisplaySelectProjectView(AppData& app) {
  Project* current = nullptr;
  for (Project& project : projects) {
    if (project.is_opened) {
      current = &project;
      break;
    }
  }
  if (current) {
    LOG() << "Opening last opened project" << current->path;
    OpenSpecifiedProject(app, *current);
  } else {
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
      app, kViewSlot, "SelectProjectView.qml",
      {
          UIDataField{"windowTitle", "Open Project"},
          UIDataField{"vFilter", ""},
      },
      {
          UIListField{"vProjects", role_names, MakeFilteredListOfProjects()},
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
  choose_project = ScheduleProcess<ChooseFile>(app, this);
  choose_project->window_title = "Open Project";
  choose_project->choose_directory = true;
  EXEC_NEXT(HandleOpenNewProjectCompletion);
}

void SelectProject::HandleOpenNewProjectCompletion(AppData& app) {
  if (!choose_project->result.isEmpty()) {
    Project* existing_project = nullptr;
    for (Project& project : projects) {
      if (project.path == choose_project->result) {
        existing_project = &project;
        break;
      }
    }
    if (existing_project) {
      OpenSpecifiedProject(app, *existing_project);
    } else {
      Project project;
      project.id = QUuid::createUuid();
      project.path = choose_project->result;
      project.is_opened = true;
      project.last_open_time = QDateTime::currentDateTime();
      ExecDbCmdOnIOThread(app, "INSERT INTO project VALUES(?, ?, ?, ?)",
                          {project.id, project.path, project.is_opened,
                           project.last_open_time});
      OpenSpecifiedProject(app, project);
    }
  } else {
    DisplaySelectProjectView(app);
  }
}

void SelectProject::OpenExistingProject(AppData& app) {
  int i = GetEventArg(app, 0).toInt();
  OpenSpecifiedProject(app, projects[i]);
}

void SelectProject::FilterProjects(AppData& app) {
  filter = GetEventArg(app, 0).toString();
  QList<QVariantList> projects = MakeFilteredListOfProjects();
  GetUIListField(app, kViewSlot, "vProjects").SetItems(projects);
  EXEC_NEXT(KeepAlive);
}

void SelectProject::RemoveProject(AppData& app) {
  int i = GetEventArg(app, 0).toInt();
  ExecDbCmdOnIOThread(app, kSqlDeleteProject, {projects[i].id});
  projects.remove(i);
  QList<QVariantList> projects = MakeFilteredListOfProjects();
  GetUIListField(app, kViewSlot, "vProjects").SetItems(projects);
  EXEC_NEXT(KeepAlive);
}

QList<QVariantList> SelectProject::MakeFilteredListOfProjects() {
  QList<QVariantList> rows;
  for (int i = 0; i < projects.size(); i++) {
    Project& project = projects[i];
    AppendToUIListIfMatches(
        rows, filter,
        {i, project.GetFolderName(), project.GetPathRelativeToHome()}, {1, 2});
  }
  return rows;
}
