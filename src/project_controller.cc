#include "project_controller.h"

#include "application.h"
#include "database.h"
#include "qvariant_list_model.h"

#define LOG() qDebug() << "[ProjectController]"

const QString kSqlDeleteProject = "DELETE FROM project WHERE id=?";

ProjectListModel::ProjectListModel(QObject* parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "idx"}, {1, "title"}, {2, "subTitle"}});
  searchable_roles = {1, 2};
}

QVariantList ProjectListModel::GetRow(int i) const {
  const Project& project = list[i];
  return {i, project.GetFolderName(), project.GetPathRelativeToHome()};
}

int ProjectListModel::GetRowCount() const { return list.size(); }

ProjectController::ProjectController(QObject* parent)
    : QObject(parent), projects(new ProjectListModel(this)) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Open Project");
  app.RunIOTask<QList<Project>>(
      this,
      []() {
        LOG() << "Loading projects from datbaase";
        Database::Transaction t;
        QList<Project> projects = Database::ExecQueryAndRead<Project>(
            "SELECT * FROM project ORDER BY last_open_time DESC",
            &ProjectSystem::ReadProjectFromSql);
        LOG() << projects.size() << "projects found";
        QList<Project> filtered;
        for (const Project& project : projects) {
          if (QFile(project.path).exists()) {
            filtered.append(project);
            continue;
          }
          LOG() << "Project" << project.path << "no longer exists - removing";
          Database::ExecCmd(kSqlDeleteProject, {project.id});
        }
        LOG() << filtered.size() << "projects still exist";
        return filtered;
      },
      [this](QList<Project> result) {
        projects->list = result;
        Project* current = nullptr;
        for (Project& project : projects->list) {
          if (project.is_opened) {
            current = &project;
          }
        }
        if (current) {
          LOG() << "Opening last opened project" << current->path;
          Application::Get().project.SetCurrentProject(*current);
        } else {
          projects->Load();
          emit selectProject();
        }
      });
}

void ProjectController::DeleteProject(int i) {
  const Project& project = projects->list[i];
  LOG() << "Deleting project" << project.path;
  Database::ExecCmdAsync(kSqlDeleteProject, {project.id});
  projects->list.remove(i);
  projects->Load();
}

void ProjectController::OpenProject(int i) {
  Project& project = projects->list[i];
  LOG() << "Opening project" << project.path;
  Application::Get().project.SetCurrentProject(project);
}

void ProjectController::OpenNewProject(const QString& path) {
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