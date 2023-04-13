#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QtQmlIntegration>

#include "project_system.h"
#include "qvariant_list_model.h"

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

  ProjectListModel* projects;

 public slots:
  void deleteProject(int i);
  void openProject(int i);
  void openNewProject(const QString& path);

 signals:
  void selectProject();
};
