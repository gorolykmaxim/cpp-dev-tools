#pragma once

#include <QObject>
#include <functional>

#include "UserCommandListModel.hpp"

class UserCommandController : public QObject {
  Q_OBJECT
  Q_PROPERTY(UserCommandListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  UserCommandController();
  void RegisterCommands();
  const QList<UserCommand>& GetUserCommands() const;

 public slots:
  void ExecuteCommand(int i);

 private:
  void RegisterCommand(const QString& group, const QString& name,
                       const QString& shortcut,
                       const std::function<void()>& callback);
  UserCommandListModel* user_commands;
};
