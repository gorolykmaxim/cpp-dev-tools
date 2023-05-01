#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QtQmlIntegration>

#include "project_system.h"
#include "qvariant_list_model.h"

class ProjectListModel : public QVariantListModel {
 public:
  explicit ProjectListModel(QObject* parent, const Project& selected);

  QList<Project> list;
  const Project& selected;

 protected:
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
};

class ProjectController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(ProjectListModel* projects MEMBER projects CONSTANT)
 public:
  explicit ProjectController(QObject* parent = nullptr);

  ProjectListModel* projects;

 public slots:
  void selectProject(int i);
  void deleteSelectedProject();
  void openSelectedProject();
  void openNewProject(const QString& path);
  void openNewProject();
  void updateProjectPath();
  void updateSelectedProjectPath(const QString& path);
  void displayProjects();

 signals:
  void displayList();
  void displayOpenNewProjectView();
  void displayUpdateExistingProjectView();

 private:
  Project selected;
};
