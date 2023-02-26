#include "ProjectController.hpp"

#include "Application.hpp"
#include "Database.hpp"
#include "Project.hpp"
#include "QVariantListModel.hpp"

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
  Application::Get().RunIOTask<QList<Project>>(
      this,
      []() {
        LOG() << "Loading projects from datbaase";
        Database::Transaction t;
        QList<Project> projects = Database::ExecQueryAndRead<Project>(
            "SELECT * FROM project ORDER BY last_open_time DESC",
            &Project::ReadFromSql);
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
        projects->Load();
      });
}

void ProjectController::DeleteProject(int i) {
  QUuid id = projects->list[i].id;
  LOG() << "Deleting project" << id;
  Database::ExecCmdAsync(kSqlDeleteProject, {id});
  projects->list.remove(i);
  projects->Load();
}

void ProjectController::OpenProject(int i) {
  Project& project = projects->list[i];
  LOG() << "Opening project" << project.id;
  OpenProject(project);
}

void ProjectController::OpenNewProject(const QString& path) {
  LOG() << "Creating new project" << path;
  Project* existing = nullptr;
  for (Project& project : projects->list) {
    if (project.path == path) {
      LOG() << "Attempting to re-open an existing project" << project.id;
      existing = &project;
      break;
    }
  }
  if (existing) {
    OpenProject(*existing);
  } else {
    Project project;
    project.id = QUuid::createUuid();
    project.path = path;
    project.is_opened = true;
    project.last_open_time = QDateTime::currentDateTime();
    Database::ExecCmdAsync(
        "INSERT INTO project VALUES(?, ?, ?, ?)",
        {project.id, project.path, project.is_opened, project.last_open_time});
    OpenProject(project);
  }
}

void ProjectController::OpenProject(Project& project) {
  project.is_opened = true;
  project.last_open_time = QDateTime::currentDateTime();
  Application::Get().current_project = QSharedPointer<Project>::create(project);
  QDir::setCurrent(project.path);
  Database::ExecCmdAsync(
      "UPDATE project SET is_opened=?, last_open_time=? WHERE id=?",
      {project.is_opened, project.last_open_time, project.id});
}
