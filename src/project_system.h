#pragma once

#include <QDateTime>
#include <QObject>
#include <QSqlQuery>
#include <QUuid>

struct Project {
  QUuid id;
  QString path;
  bool is_opened = false;
  QDateTime last_open_time;

  QString GetPathRelativeToHome() const;
  QString GetFolderName() const;
  bool IsNull() const;
  bool operator==(const Project& other) const;
  bool operator!=(const Project& other) const;
};

class ProjectSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      QString currentProjectShortPath READ GetCurrentProjectPathRelativeToHome
          NOTIFY currentProjectChanged)
  Q_PROPERTY(
      bool isProjectOpened READ IsProjectOpened NOTIFY currentProjectChanged)
 public:
  static Project ReadProjectFromSql(QSqlQuery& sql);
  void SetCurrentProject(Project project);
  const Project& GetCurrentProject() const;
  QString GetCurrentProjectPathRelativeToHome() const;
  bool IsProjectOpened() const;
 signals:
  void currentProjectChanged();

 private:
  Project current_project;
};
