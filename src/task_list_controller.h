#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "qvariant_list_model.h"
#include "task_system.h"

class TaskListModel : public QVariantListModel {
 public:
  explicit TaskListModel(QObject* parent);

 protected:
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
};

class TaskListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      bool showPlaceholder READ ShouldShowPlaceholder NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isLoading READ IsLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(TaskListModel* tasks MEMBER tasks CONSTANT)
 public:
  explicit TaskListController(QObject* parent = nullptr);
  bool ShouldShowPlaceholder() const;
  bool IsLoading() const;

  TaskListModel* tasks;

 signals:
  void isLoadingChanged();

 private:
  void SetIsLoading(bool value);

  bool is_loading;
};
