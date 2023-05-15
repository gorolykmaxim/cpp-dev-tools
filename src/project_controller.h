#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QtQmlIntegration>

#include "project_system.h"
#include "text_list_model.h"

class ProjectListModel : public TextListModel {
 public:
  explicit ProjectListModel(QObject* parent);
  Project GetSelected() const;

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

  ProjectListModel* projects;

 public slots:
  void deleteSelectedProject();
  void openSelectedProject();
  void openProject(const QString& path);
  void displayOpenProject();
  void displayChangeProjectPath();
  void changeSelectedProjectPath(const QString& path);
  void displayProjectList();

 signals:
  void displayProjectListView();
  void displayOpenProjectView();
  void displayChangeProjectPathView();
};
