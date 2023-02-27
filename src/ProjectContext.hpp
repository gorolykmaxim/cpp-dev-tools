#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "Project.hpp"

class ProjectContext : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(Project currentProject MEMBER current_project NOTIFY
                 currentProjectChanged)
 public slots:
  void CloseProject();
 signals:
  void currentProjectChanged();

 private:
  Project current_project;
};
