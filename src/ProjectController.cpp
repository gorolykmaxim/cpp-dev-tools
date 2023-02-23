#include "ProjectController.hpp"

#include <QMetaObject>
#include <QSqlQuery>
#include <QtConcurrent>

#include "Application.hpp"
#include "Database.hpp"
#include "QVariantListModel.hpp"

class Project {
 public:
  static Project ReadFromSql(QSqlQuery& sql) {
    Project project;
    project.id = sql.value(0).toUuid();
    project.path = sql.value(1).toString();
    project.is_opened = sql.value(2).toBool();
    project.last_open_time = sql.value(3).toDateTime();
    return project;
  }

  QString GetPathRelativeToHome() const {
    QString home_str =
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (path.startsWith(home_str)) {
      return "~/" + QDir(home_str).relativeFilePath(path);
    } else {
      return path;
    }
  }

  QString GetFolderName() const {
    qsizetype i = path.lastIndexOf('/');
    return i < 0 ? path : path.sliced(i + 1);
  }

  QUuid id;
  QString path;
  bool is_opened = false;
  QDateTime last_open_time;
};

ProjectController::ProjectController(QObject* parent) : QObject(parent) {
  Application& app = Application::Get();
  QHash<int, QByteArray> role_names = {{0, "title"}, {1, "subTitle"}};
  QList<int> searchable_roles = {0, 1};
  projects =
      QSharedPointer<QVariantListModel>::create(role_names, searchable_roles);
  app.RunIOTask<QList<Project>>(
      []() {
        return Database::ExecQueryAndRead<Project>(
            "SELECT * FROM project ORDER BY last_open_time DESC",
            &Project::ReadFromSql);
      },
      [this](QList<Project> results) {
        QList<QVariantList> rows;
        for (const Project& project : results) {
          rows.append(
              {project.GetFolderName(), project.GetPathRelativeToHome()});
        }
        projects->SetItems(rows);
      });
}

QVariantListModel* ProjectController::GetProjects() { return projects.get(); }
