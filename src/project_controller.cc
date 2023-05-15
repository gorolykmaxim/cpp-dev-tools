#include "project_controller.h"

#include <algorithm>

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "text_list_model.h"
#include "theme.h"
#include "ui_icon.h"

#define LOG() qDebug() << "[ProjectController]"

ProjectListModel::ProjectListModel(QObject* parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"},
                {1, "subTitle"},
                {2, "existsOnDisk"},
                {3, "titleColor"},
                {4, "icon"},
                {5, "iconColor"}});
  searchable_roles = {0, 1};
}

Project ProjectListModel::GetSelected() const {
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return Project();
  } else {
    return list[i];
  }
}

QVariantList ProjectListModel::GetRow(int i) const {
  static const Theme kTheme;
  const Project& project = list[i];
  QString title_color;
  UiIcon icon;
  QString name = project.GetFolderName();
  if (project.is_missing_on_disk) {
    title_color = kTheme.kColorSubText;
    icon.icon = "not_interested";
    icon.color = title_color;
    name = "[Not Found] " + name;
  } else {
    icon.icon = "widgets";
  }
  return {name,
          project.GetPathRelativeToHome(),
          !project.is_missing_on_disk,
          title_color,
          icon.icon,
          icon.color};
}

int ProjectListModel::GetRowCount() const { return list.size(); }

static bool Compare(const Project& a, const Project& b) {
  if (a.is_missing_on_disk != b.is_missing_on_disk) {
    return a.is_missing_on_disk < b.is_missing_on_disk;
  } else {
    return a.last_open_time > b.last_open_time;
  }
}

ProjectController::ProjectController(QObject* parent)
    : QObject(parent), projects(new ProjectListModel(this)) {}

void ProjectController::deleteSelectedProject() {
  Project selected = projects->GetSelected();
  if (selected.IsNull()) {
    return;
  }
  LOG() << "Deleting project" << selected.path;
  Database::ExecCmdAsync("DELETE FROM project WHERE id=?", {selected.id});
  projects->list.removeAll(selected);
  projects->Load();
}

void ProjectController::openSelectedProject() {
  Project selected = projects->GetSelected();
  if (selected.IsNull() || selected.is_missing_on_disk) {
    return;
  }
  LOG() << "Opening project" << selected.path;
  Application::Get().project.SetCurrentProject(selected);
}

void ProjectController::openProject(const QString& path) {
  LOG() << "Creating new project" << path;
  Project* existing = nullptr;
  for (Project& project : projects->list) {
    if (project.path == path) {
      LOG() << "Attempting to re-open an existing project" << project.path;
      existing = &project;
      break;
    }
  }
  if (existing) {
    Application::Get().project.SetCurrentProject(*existing);
  } else {
    Project project;
    project.id = QUuid::createUuid();
    project.path = path;
    project.is_opened = true;
    project.last_open_time = QDateTime::currentDateTime();
    Database::ExecCmdAsync(
        "INSERT INTO project VALUES(?, ?, ?, ?)",
        {project.id, project.path, project.is_opened, project.last_open_time});
    Application::Get().project.SetCurrentProject(project);
  }
}

void ProjectController::displayOpenProject() {
  Application::Get().view.SetWindowTitle("Open Project");
  emit displayOpenProjectView();
}

void ProjectController::displayChangeProjectPath() {
  if (projects->GetSelected().IsNull()) {
    return;
  }
  Application::Get().view.SetWindowTitle("Change Project's Path");
  emit displayChangeProjectPathView();
}

void ProjectController::changeSelectedProjectPath(const QString& path) {
  Project selected = projects->GetSelected();
  LOG() << "Updating path of project" << selected.id << "to" << path;
  selected.path = path;
  Database::ExecCmdAsync("UPDATE project SET path=? WHERE id=?",
                         {selected.path, selected.id});
  displayProjectList();
}

void ProjectController::displayProjectList() {
  Application::Get().view.SetWindowTitle("Open Project");
  IoTask::Run<QList<Project>>(
      this,
      []() {
        LOG() << "Loading projects from datbaase";
        Database::Transaction t;
        QList<Project> projects = Database::ExecQueryAndRead<Project>(
            "SELECT * FROM project", &ProjectSystem::ReadProjectFromSql);
        LOG() << projects.size() << "projects found";
        for (Project& project : projects) {
          project.is_missing_on_disk = !QFile(project.path).exists();
          if (project.is_missing_on_disk) {
            project.is_opened = false;
            Database::ExecCmd("UPDATE project SET is_opened=false WHERE id=?",
                              {project.id});
          }
        }
        std::sort(projects.begin(), projects.end(), Compare);
        return projects;
      },
      [this](QList<Project> result) {
        projects->list = result;
        Project* current = nullptr;
        for (Project& project : projects->list) {
          if (project.is_opened && !project.is_missing_on_disk) {
            current = &project;
          }
        }
        if (current) {
          LOG() << "Opening last opened project" << current->path;
          Application::Get().project.SetCurrentProject(*current);
        } else {
          projects->Load();
          emit displayProjectListView();
        }
      });
}
