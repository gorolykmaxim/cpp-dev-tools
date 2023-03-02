#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QtQmlIntegration>

#include "Project.hpp"
#include "QVariantListModel.hpp"

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
  Q_PROPERTY(ProjectListModel* projects MEMBER projects CONSTANT)
 public:
  explicit ProjectController(QObject* parent = nullptr);

 public slots:
  void LoadProjects();
  void DeleteProject(int i);
  void OpenProject(int i);
  void OpenNewProject(const QString& path);

 signals:
  void selectProject();
  void projectSelected(const Project& project);

 private:
  void OpenProject(Project& project);

  ProjectListModel* projects;
};
