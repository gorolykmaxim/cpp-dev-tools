#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>

class QVariantListModel : public QAbstractListModel {
  Q_OBJECT
 public:
  explicit QVariantListModel(const QHash<int, QByteArray>& role_names);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role = 0) const override;
  void SetItems(const QList<QVariantList>& items);
 public slots:
  QVariant GetFieldByRoleName(int row, const QString& name) const;
  QVariant GetFieldByRole(int row, int role) const;

 private:
  QList<QVariantList> items;
  QHash<int, QByteArray> role_names;
  QHash<QString, int> name_to_role;
};
