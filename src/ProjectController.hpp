#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"

class Project {
 public:
  static Project ReadFromSql(QSqlQuery& sql);
  QString GetPathRelativeToHome() const;
  QString GetFolderName() const;

  QUuid id;
  QString path;
  bool is_opened = false;
  QDateTime last_open_time;
};

class ProjectController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QVariantListModel* projects READ GetProjects CONSTANT)
 public:
  explicit ProjectController(QObject* parent = nullptr);
  QVariantListModel* GetProjects();

 public slots:
  void DeleteProject(int i);

 private:
  QVariantList GetProjectUIData(int i) const;

  QList<Project> projects;
  QSharedPointer<QVariantListModel> projects_model;
};
