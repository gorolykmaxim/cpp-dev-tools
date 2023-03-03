#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"

class SearchUserCommandController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      SimpleQVariantListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  explicit SearchUserCommandController(QObject* parent = nullptr);
 public slots:
  void LoadUserCommands();
  void ExecuteCommand(int i);

 signals:
  void commandExecuted();

 private:
  SimpleQVariantListModel* user_commands;
};
