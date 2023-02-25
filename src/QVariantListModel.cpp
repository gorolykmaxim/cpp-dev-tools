#include "QVariantListModel.hpp"

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

void QVariantListModel::Load() {
  beginResetModel();
  items.clear();
  int count = GetRowCount();
  for (int i = 0; i < count; i++) {
    QVariantList row = GetRow(i);
    if (filter.isEmpty() || searchable_roles.isEmpty()) {
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

void QVariantListModel::SetFilterIfChanged(const QString& filter) {
  if (filter == this->filter) {
    return;
  }
  SetFilter(filter);
}

void QVariantListModel::SetFilter(const QString& filter) {
  this->filter = filter;
  emit filterChanged();
  Load();
}

QString QVariantListModel::GetFilter() { return filter; }

QVariantList QVariantListModel::GetRow(int) const { return {}; }

int QVariantListModel::GetRowCount() const { return 0; }

void QVariantListModel::SetRoleNames(const QHash<int, QByteArray>& role_names) {
  this->role_names = role_names;
  for (int role : role_names.keys()) {
    QString name(role_names[role]);
    name_to_role[name] = role;
  }
}
