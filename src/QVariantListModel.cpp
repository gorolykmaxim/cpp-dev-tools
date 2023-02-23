#include "QVariantListModel.hpp"

QVariantListModel::QVariantListModel(const QHash<int, QByteArray>& role_names,
                                     const QList<int> searchable_roles)
    : QAbstractListModel(),
      searchable_roles(searchable_roles),
      role_names(role_names) {
  for (int role : role_names.keys()) {
    QString name(role_names[role]);
    name_to_role[name] = role;
  }
}

int QVariantListModel::rowCount(const QModelIndex&) const {
  return items_filtered.size();
}

QHash<int, QByteArray> QVariantListModel::roleNames() const {
  return role_names;
}

QVariant QVariantListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  if (index.row() < 0 || index.row() >= items_filtered.size()) {
    return QVariant();
  }
  const QVariantList& row = items_filtered[index.row()];
  Q_ASSERT(role >= 0 && role < row.size());
  return row[role];
}

void QVariantListModel::SetItems(const QList<QVariantList>& items) {
  this->items = items;
  Filter();
}

QVariant QVariantListModel::GetFieldByRoleName(int row,
                                               const QString& name) const {
  if (!name_to_role.contains(name)) {
    return QVariant();
  }
  return GetFieldByRole(row, name_to_role[name]);
}

QVariant QVariantListModel::GetFieldByRole(int row, int role) const {
  if (row < 0 || row >= items_filtered.size()) {
    return QVariant();
  }
  const QVariantList& row_values = items_filtered[row];
  if (role < 0 || role >= row_values.size()) {
    return QVariant();
  }
  return row_values[role];
}

void QVariantListModel::SetFilter(const QString& filter) {
  if (filter == this->filter) {
    return;
  }
  this->filter = filter;
  emit filterChanged();
  Filter();
}

QString QVariantListModel::GetFilter() { return filter; }

void QVariantListModel::Filter() {
  beginResetModel();
  items_filtered.clear();
  for (QVariantList row : items) {
    if (filter.isEmpty()) {
      items_filtered.append(row);
      continue;
    }
    bool matches = false;
    for (int role : searchable_roles) {
      QString str = row[role].toString();
      int i = str.indexOf(filter, Qt::CaseSensitivity::CaseInsensitive);
      if (i >= 0) {
        str.insert(i + filter.size(), "</b>");
        str.insert(i, "<b>");
        row[role] = str;
        matches = true;
      }
    }
    if (matches) {
      items_filtered.append(row);
    }
  }
  endResetModel();
}
