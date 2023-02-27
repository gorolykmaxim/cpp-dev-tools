#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "UserCommandListModel.hpp"

class UserCommandController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(UserCommandListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  explicit UserCommandController(QObject* parent = nullptr);
 public slots:
  void RegisterCommand(const QString& group, const QString& name,
                       const QString& shortcut, QVariant callback);
  void Commit();

 private:
  UserCommandListModel* user_commands;
};
