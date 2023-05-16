#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "text_list_model.h"

class TaskListModel : public TextListModel {
 public:
  explicit TaskListModel(QObject* parent);

 protected:
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
};

class TaskListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(int selectedTaskIndex READ GetSelectedTaskIndex NOTIFY
                 selectedTaskIndexChanged)
  Q_PROPERTY(TaskListModel* tasks MEMBER tasks CONSTANT)
 public:
  explicit TaskListController(QObject* parent = nullptr);
  int GetSelectedTaskIndex() const;

  TaskListModel* tasks;

 signals:
  void selectedTaskIndexChanged();
};
