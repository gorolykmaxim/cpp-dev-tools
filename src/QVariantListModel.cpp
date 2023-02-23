#include "QVariantListModel.hpp"

QVariantListModel::QVariantListModel(const GetRow& get_row,
                                     const GetRowCount& get_row_count,
                                     const QHash<int, QByteArray>& role_names,
                                     const QList<int> searchable_roles)
    : QAbstractListModel(),
      get_row(get_row),
      get_row_count(get_row_count),
      searchable_roles(searchable_roles),
      role_names(role_names) {
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

void QVariantListModel::LoadItems() {
  beginResetModel();
  items.clear();
  int count = get_row_count();
  for (int i = 0; i < count; i++) {
    QVariantList row = get_row(i);
    if (filter.isEmpty()) {
      items.append(row);
      continue;
    }
    bool matches = false;
    for (int role : searchable_roles) {
      QString str = row[role].toString();
      int pos = str.indexOf(filter, Qt::CaseSensitivity::CaseInsensitive);
      if (pos >= 0) {
        str.insert(pos + filter.size(), "</b>");
        str.insert(pos, "<b>");
        row[role] = str;
        matches = true;
      }
    }
    if (matches) {
      items.append(row);
    }
  }
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

void QVariantListModel::SetFilter(const QString& filter) {
  if (filter == this->filter) {
    return;
  }
  this->filter = filter;
  emit filterChanged();
  LoadItems();
}

QString QVariantListModel::GetFilter() { return filter; }
