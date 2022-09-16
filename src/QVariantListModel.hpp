#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QModelIndex>
#include <QVariant>
#include <QVector>

class QVariantListModel: public QAbstractListModel {
public:
  explicit QVariantListModel(const QHash<int, QByteArray>& role_names);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role = 0) const override;
  void SetItems(const QVector<QVariantList>& items);
private:
  QVector<QVariantList> items;
  QHash<int, QByteArray> role_names;
};
