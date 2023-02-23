#include "QVariantListModel.hpp"

QVariantListModel::QVariantListModel(const QHash<int, QByteArray>& role_names)
    : QAbstractListModel(), role_names(role_names) {
  for (int role : role_names.keys()) {
    QString name(role_names[role]);
    name_to_role[name] = role;
  }
}

int QVariantListModel::rowCount(const QModelIndex&) const {
  return items.size();
}

QHash<int, QByteArray> QVariantListModel::roleNames() const {
  return role_names;
}

QVariant QVariantListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  if (index.row() < 0 || index.row() >= items.size()) {
    return QVariant();
  }
  const QVariantList& row = items[index.row()];
  Q_ASSERT(role >= 0 && role < row.size());
  return row[role];
}

void QVariantListModel::SetItems(const QList<QVariantList>& items) {
  beginResetModel();
  this->items = items;
  endResetModel();
}

QVariant QVariantListModel::GetFieldByRoleName(int row,
                                               const QString& name) const {
  if (!name_to_role.contains(name)) {
    return QVariant();
  }
  return GetFieldByRole(row, name_to_role[name]);
}

QVariant QVariantListModel::GetFieldByRole(int row, int role) const {
  if (row < 0 || row >= items.size()) {
    return QVariant();
  }
  const QVariantList& row_values = items[row];
  if (role < 0 || role >= row_values.size()) {
    return QVariant();
  }
  return row_values[role];
}
