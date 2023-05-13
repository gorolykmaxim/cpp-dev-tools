#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QtQmlIntegration>

#include "project_system.h"
#include "text_list_model.h"

class ProjectListModel : public TextListModel {
 public:
  explicit ProjectListModel(QObject* parent, Project& selected);

  QList<Project> list;
  Project& selected;

 protected:
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
};

class ProjectController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(ProjectListModel* projects MEMBER projects CONSTANT)
  Q_PROPERTY(bool isProjectSelected READ IsProjectSelected NOTIFY
                 selectedProjectChanged)
 public:
  explicit ProjectController(QObject* parent = nullptr);
  bool IsProjectSelected() const;

  ProjectListModel* projects;

 public slots:
  void selectProject(int i);
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
  void selectedProjectChanged();

 private:
  Project selected;
};
