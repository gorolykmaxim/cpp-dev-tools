#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "text_list_model.h"

class SearchUserCommandListModel : public TextListModel {
 public:
  explicit SearchUserCommandListModel(QObject* parent);

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

class SearchUserCommandController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      SearchUserCommandListModel* userCommands MEMBER user_commands CONSTANT)
 public:
  explicit SearchUserCommandController(QObject* parent = nullptr);
 public slots:
  void loadUserCommands();
  void executeSelectedCommand();

 signals:
  void commandExecuted();

 private:
  SearchUserCommandListModel* user_commands;
};
