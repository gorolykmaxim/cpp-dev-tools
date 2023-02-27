#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "Project.hpp"
#include "QVariantListModel.hpp"

class ChooseTaskController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      bool showPlaceholder READ ShouldShowPlaceholder NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isLoading READ IsLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(SimpleQVariantListModel* tasks MEMBER tasks CONSTANT)
  Q_PROPERTY(Project project MEMBER project NOTIFY projectChanged)
 public:
  explicit ChooseTaskController(QObject* parent = nullptr);
  bool ShouldShowPlaceholder() const;
  bool IsLoading() const;

 public slots:
  void FindTasks();

 signals:
  void projectChanged();
  void isLoadingChanged();

 private:
  void SetIsLoading(bool value);

  bool is_loading;
  SimpleQVariantListModel* tasks;
  Project project;
};
