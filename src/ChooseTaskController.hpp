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
 public:
  explicit ChooseTaskController(QObject* parent = nullptr);
  bool ShouldShowPlaceholder() const;
  bool IsLoading() const;

  SimpleQVariantListModel* tasks;

 public slots:
  void ExecTask(int i);

 signals:
  void isLoadingChanged();

 private:
  void SetIsLoading(bool value);

  bool is_loading;
};
