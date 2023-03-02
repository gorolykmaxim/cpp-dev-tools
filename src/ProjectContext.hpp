#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "Project.hpp"

class ProjectContext : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(Project currentProject MEMBER current_project NOTIFY
                 currentProjectChanged)
  Q_PROPERTY(
      QString currentProjectShortPath READ GetCurrentProjectPathRelativeToHome
          NOTIFY currentProjectChanged)
  Q_PROPERTY(
      bool isProjectOpened READ IsProjectOpened NOTIFY currentProjectChanged)
 public:
  void SetCurrentProject(const Project& project);
  Project GetCurrentProject() const;
  QString GetCurrentProjectPathRelativeToHome() const;
  bool IsProjectOpened() const;
 public slots:
  void CloseProject();
 signals:
  void currentProjectChanged();

 private:
  Project current_project;
};
