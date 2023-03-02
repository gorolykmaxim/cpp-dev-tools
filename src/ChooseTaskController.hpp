#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "Project.hpp"
#include "ProjectContext.hpp"
#include "QVariantListModel.hpp"

class ChooseTaskController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      bool showPlaceholder READ ShouldShowPlaceholder NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isLoading READ IsLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(ProjectContext* mProjectContext MEMBER project_context NOTIFY
                 projectContextChanged)
  Q_PROPERTY(SimpleQVariantListModel* tasks MEMBER tasks CONSTANT)
 public:
  explicit ChooseTaskController(QObject* parent = nullptr);
  bool ShouldShowPlaceholder() const;
  bool IsLoading() const;

  ProjectContext* project_context;
  SimpleQVariantListModel* tasks;

 public slots:
  void FindTasks();

 signals:
  void projectContextChanged();
  void isLoadingChanged();

 private:
  void SetIsLoading(bool value);

  bool is_loading;
};
