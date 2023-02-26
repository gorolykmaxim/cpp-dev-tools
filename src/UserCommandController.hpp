#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"

class UserCommandListModel : public QVariantListModel {
 public:
  explicit UserCommandListModel(QObject* parent);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;

  QList<QVariantList> list;
};

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
