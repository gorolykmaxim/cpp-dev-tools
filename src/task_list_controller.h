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
  Q_PROPERTY(
      bool showPlaceholder READ ShouldShowPlaceholder NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isLoading READ IsLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(int selectedTaskIndex READ GetSelectedTaskIndex NOTIFY
                 selectedTaskIndexChanged)
  Q_PROPERTY(TaskListModel* tasks MEMBER tasks CONSTANT)
 public:
  explicit TaskListController(QObject* parent = nullptr);
  bool ShouldShowPlaceholder() const;
  bool IsLoading() const;
  int GetSelectedTaskIndex() const;

  TaskListModel* tasks;

 signals:
  void isLoadingChanged();
  void selectedTaskIndexChanged();

 private:
  void SetIsLoading(bool value);

  bool is_loading;
};
