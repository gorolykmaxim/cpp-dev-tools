#pragma once

#include <QObject>
#include <QSqlQuery>
#include <functional>

#include "text_list_model.h"

using UserCommandIndex = QHash<QString, QHash<QString, QString>>;

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
  static UserCommand ReadUserCommandFromSql(QSqlQuery& sql);
  void Initialize();
  const QList<GlobalUserCommand>& GetUserCommands() const;
  void ReloadCommands();

 public slots:
  void executeCommand(int i);
  QString getShortcut(const QString& group, const QString& name) const;

 private:
  void UpdateGlobalShortcuts(const QList<UserCommand>& cmds);
  void UpdateLocalShortcuts(const QList<UserCommand>& cmds);

  GlobalUserCommandListModel* user_commands;
  UserCommandIndex user_cmd_index;
};
