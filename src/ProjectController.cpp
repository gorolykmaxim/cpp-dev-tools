#include "ProjectController.hpp"

#include <QMetaObject>
#include <QtConcurrent>

#include "Application.hpp"
#include "Database.hpp"
#include "QVariantListModel.hpp"

#define LOG() qDebug() << "[ProjectController]"

const QString kSqlDeleteProject = "DELETE FROM project WHERE id=?";

Project Project::ReadFromSql(QSqlQuery& sql) {
  Project project;
  project.id = sql.value(0).toUuid();
  project.path = sql.value(1).toString();
  project.is_opened = sql.value(2).toBool();
  project.last_open_time = sql.value(3).toDateTime();
  return project;
}

QString Project::GetPathRelativeToHome() const {
  QString home_str =
      QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  if (path.startsWith(home_str)) {
    return "~/" + QDir(home_str).relativeFilePath(path);
  } else {
    return path;
  }
}

QString Project::GetFolderName() const {
  qsizetype i = path.lastIndexOf('/');
  return i < 0 ? path : path.sliced(i + 1);
}

ProjectController::ProjectController(QObject* parent) : QObject(parent) {
  Application& app = Application::Get();
  QHash<int, QByteArray> role_names = {
      {0, "idx"}, {1, "title"}, {2, "subTitle"}};
  QList<int> searchable_roles = {1, 2};
  projects_model = QSharedPointer<QVariantListModel>::create(
      [this](int i) { return GetProjectUIData(i); },
      [this]() { return projects.size(); }, role_names, searchable_roles);
  app.RunIOTask<QList<Project>>(
      []() {
        Database::Transaction t;
        QList<Project> projects = Database::ExecQueryAndRead<Project>(
            "SELECT * FROM project ORDER BY last_open_time DESC",
            &Project::ReadFromSql);
        QList<Project> filtered;
        for (const Project& project : projects) {
          if (QFile(project.path).exists()) {
            filtered.append(project);
            continue;
          }
          LOG() << "Project" << project.path << "no longer exists - removing";
          Database::ExecCmd(kSqlDeleteProject, {project.id});
        }
        return filtered;
      },
      [this](QList<Project> results) {
        projects = results;
        projects_model->LoadItems();
      });
}

QVariantListModel* ProjectController::GetProjects() {
  return projects_model.get();
}

void ProjectController::DeleteProject(int i) {
  Database::ExecCmdAsync(kSqlDeleteProject, {projects[i].id});
  projects.remove(i);
  projects_model->LoadItems();
}

QVariantList ProjectController::GetProjectUIData(int i) const {
  const Project& project = projects[i];
  return {i, project.GetFolderName(), project.GetPathRelativeToHome()};
}
