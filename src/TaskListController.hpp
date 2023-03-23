#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"
#include "TaskSystem.hpp"

class TaskListModel : public QVariantListModel {
 public:
  explicit TaskListModel(QObject* parent);

  QList<Task> list;

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

 public slots:
  void ExecuteTask(int i, bool repeat_until_fail) const;

 signals:
  void isLoadingChanged();

 private:
  void SetIsLoading(bool value);

  bool is_loading;
};
