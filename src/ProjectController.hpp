#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QtQmlIntegration>

#include "Project.hpp"
#include "ProjectContext.hpp"
#include "QVariantListModel.hpp"
#include "ViewController.hpp"

class ProjectListModel : public QVariantListModel {
 public:
  explicit ProjectListModel(QObject* parent);

  QList<Project> list;

 protected:
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
};

class ProjectController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(ProjectContext* mProjectContext MEMBER project_context NOTIFY
                 projectContextChanged)
  Q_PROPERTY(ViewController* mViewController MEMBER view_controller NOTIFY
                 viewControllerChanged)
  Q_PROPERTY(ProjectListModel* projects MEMBER projects CONSTANT)
 public:
  explicit ProjectController(QObject* parent = nullptr);

  ViewController* view_controller;
  ProjectContext* project_context;
  ProjectListModel* projects;

 public slots:
  void LoadProjects();
  void DeleteProject(int i);
  void OpenProject(int i);
  void OpenNewProject(const QString& path);

 signals:
  void projectContextChanged();
  void viewControllerChanged();
  void selectProject();

 private:
  void OpenProject(Project& project);
};
