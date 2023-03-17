#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QtQmlIntegration>

#include "ProjectSystem.hpp"
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

  ProjectListModel* projects;

 public slots:
  void DeleteProject(int i);
  void OpenProject(int i);
  void OpenNewProject(const QString& path);

 signals:
  void selectProject();
};
