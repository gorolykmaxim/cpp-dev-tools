#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "text_list_model.h"

class SearchUserCommandController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      SimpleTextListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  explicit SearchUserCommandController(QObject* parent = nullptr);
 public slots:
  void loadUserCommands();
  void executeCommand(int i);

 signals:
  void commandExecuted();

 private:
  SimpleTextListModel* user_commands;
};
