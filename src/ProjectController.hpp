#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"

class ProjectController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QVariantListModel* projects READ GetProjects CONSTANT)
 public:
  explicit ProjectController(QObject* parent = nullptr);
  QVariantListModel* GetProjects();

 private:
  QSharedPointer<QVariantListModel> projects;
};
