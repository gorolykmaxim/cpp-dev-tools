#pragma once

#include <QObject>
#include <functional>

#include "text_list_model.h"

struct UserCommand {
  QString GetFormattedShortcut() const;

  QString group;
  QString name;
  QString shortcut;
};

struct GlobalUserCommand : public UserCommand {
  std::function<void()> callback;
};

class GlobalUserCommandListModel : public TextListModel {
 public:
  explicit GlobalUserCommandListModel(QObject* parent);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;

  QList<GlobalUserCommand> list;
};

class UserCommandSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      GlobalUserCommandListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  UserCommandSystem();
  void Initialize();
  const QList<GlobalUserCommand>& GetUserCommands() const;

 public slots:
  void executeCommand(int i);

 private:
  GlobalUserCommandListModel* user_commands;
};
