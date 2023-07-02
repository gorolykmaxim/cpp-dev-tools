#pragma once

#include <QObject>
#include <QtQmlIntegration>
#include <entt.hpp>

#include "text_list_model.h"

class TaskListModel : public TextListModel {
  Q_OBJECT
  QML_ELEMENT
 public:
  explicit TaskListModel(QObject* parent = nullptr);

 public slots:
  void displayTaskList();
  void executeCurrentTask(bool repeat_until_fail, const QString& view,
                          const QStringList& args);

 protected:
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;

 private:
  entt::registry registry;
  QList<entt::entity> tasks;
};
