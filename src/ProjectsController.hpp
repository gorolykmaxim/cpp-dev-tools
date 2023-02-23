#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"

class ProjectsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QVariantListModel* projects READ GetProjects CONSTANT)
 public:
  explicit ProjectsController(QObject* parent = nullptr);
  QVariantListModel* GetProjects();

  QSharedPointer<QVariantListModel> projects;
};
