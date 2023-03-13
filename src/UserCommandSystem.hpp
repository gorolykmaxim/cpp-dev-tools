#pragma once

#include <QObject>
#include <functional>

#include "QVariantListModel.hpp"
#include "UserCommand.hpp"

class UserCommandListModel : public QVariantListModel {
 public:
  explicit UserCommandListModel(QObject* parent);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;

  QList<UserCommand> list;
};

class UserCommandSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(UserCommandListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  UserCommandSystem();
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
