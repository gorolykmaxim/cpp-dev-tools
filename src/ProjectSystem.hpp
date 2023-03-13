#pragma once

#include <QObject>

#include "Project.hpp"

class ProjectSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      QString currentProjectShortPath READ GetCurrentProjectPathRelativeToHome
          NOTIFY currentProjectChanged)
  Q_PROPERTY(
      bool isProjectOpened READ IsProjectOpened NOTIFY currentProjectChanged)
 public:
  void SetCurrentProject(Project project);
  const Project& GetCurrentProject() const;
  QString GetCurrentProjectPathRelativeToHome() const;
  bool IsProjectOpened() const;
 signals:
  void currentProjectChanged();

 private:
  Project current_project;
};
