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

class ProjectListModel : public QVariantListModel {
 public:
  ProjectListModel();

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
  ~ProjectController();

 public slots:
  void DeleteProject(int i);

 private:
  ProjectListModel* projects;
};
