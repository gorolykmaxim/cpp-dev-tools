#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"

class UserCommandController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      SimpleQVariantListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  explicit UserCommandController(QObject* parent = nullptr);
 public slots:
  void RegisterCommand(const QString& group, const QString& name,
                       const QString& shortcut, QVariant callback);
  void Commit();

 private:
  SimpleQVariantListModel* user_commands;
};
