#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"
#include "UserCommandListModel.hpp"

class SearchUserCommandController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      SimpleQVariantListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  explicit SearchUserCommandController(QObject* parent = nullptr);
 public slots:
  void LoadUserCommands(UserCommandListModel* user_commands);

 private:
  SimpleQVariantListModel* user_commands;
};
