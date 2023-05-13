#pragma once

#include <QObject>
#include <functional>

#include "text_list_model.h"

struct UserCommand {
  QString GetFormattedShortcut() const;

  QString group;
  QString name;
  QString shortcut;
  std::function<void()> callback;
};

class UserCommandListModel : public TextListModel {
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
  void executeCommand(int i);

 private:
  void RegisterCommand(const QString& group, const QString& name,
                       const QString& shortcut,
                       const std::function<void()>& callback);
  UserCommandListModel* user_commands;
};
