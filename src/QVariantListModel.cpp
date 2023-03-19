#include "QVariantListModel.hpp"

#include <algorithm>
#include <limits>

QVariantListModel::QVariantListModel(QObject* parent)
    : QAbstractListModel(parent) {}

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

static const QString kHighlightOpen = "<b>";
static const QString kHighlightClose = "</b>";

static QString HighlightFuzzySubString(const QString& source,
                                       const QString& term) {
  QString result;
  QTextStream stream(&result);
  qsizetype last_char_index = -1;
  bool matches = false;
  bool highlighting = false;
  for (QChar c : term) {
    qsizetype pos = source.indexOf(c, last_char_index + 1,
                                   Qt::CaseSensitivity::CaseInsensitive);
    if (pos < 0) {
      matches = false;
      break;
    }
    qsizetype distance = pos - last_char_index;
    if (distance > 1) {
      if (highlighting) {
        highlighting = false;
        stream << kHighlightClose;
      }
      stream << source.sliced(last_char_index + 1, distance - 1);
    }
    if (!highlighting) {
      highlighting = true;
      matches = true;
      stream << kHighlightOpen;
    }
    stream << source[pos];
    last_char_index = pos;
  }
  if (matches && last_char_index < source.length() - 1) {
    if (highlighting) {
      highlighting = false;
      stream << kHighlightClose;
    }
    stream << source.sliced(last_char_index + 1,
                            source.length() - last_char_index - 1);
  }
  if (matches) {
    stream.flush();
  } else {
    result = source;
  }
  return result;
}

struct SortableProps {
  int not_matching_chars = 0;
  qsizetype first_match_pos = -1;
};

static SortableProps GetSortableProps(const QString& str) {
  SortableProps result;
  qsizetype last_end = 0;
  while (true) {
    qsizetype start = str.indexOf(kHighlightOpen, last_end);
    if (start < 0) {
      break;
    }
    if (result.first_match_pos < 0) {
      result.first_match_pos = start;
    }
    result.not_matching_chars += start - last_end;
    qsizetype end =
        str.indexOf(kHighlightClose, start + kHighlightOpen.length());
    if (end < 0) {
      break;
    }
    last_end = end + kHighlightClose.length();
  }
  if (last_end == 0) {
    result.not_matching_chars = std::numeric_limits<int>::max();
    result.first_match_pos = std::numeric_limits<qsizetype>::max();
  } else if (last_end < str.length() - 1) {
    result.not_matching_chars += str.length() - last_end - 1;
  }
  return result;
}

static bool CompareRows(const QVariantList& row1, const QVariantList& row2,
                        const QList<int>& searchable_roles) {
  for (int role : searchable_roles) {
    QString a = row1[role].toString();
    QString b = row2[role].toString();
    SortableProps props1 = GetSortableProps(a);
    SortableProps props2 = GetSortableProps(b);
    if (props1.first_match_pos != props2.first_match_pos) {
      return props1.first_match_pos < props2.first_match_pos;
    } else if (props1.not_matching_chars != props2.not_matching_chars) {
      return props1.not_matching_chars < props2.not_matching_chars;
    }
  }
  return false;
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
      QString result = HighlightFuzzySubString(str, filter);
      if (result != str) {
        row[role] = result;
        matches = true;
      }
    }
    if (matches) {
      items.append(row);
    }
  }
  if (!filter.isEmpty()) {
    // Only sort when actual filter is applied. Otherwise - preserve original
    // order, supplied by the client code.
    std::sort(items.begin(), items.end(),
              [this](const QVariantList& row1, const QVariantList& row2) {
                return CompareRows(row1, row2, searchable_roles);
              });
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

SimpleQVariantListModel::SimpleQVariantListModel(
    QObject* parent, const QHash<int, QByteArray>& role_names,
    const QList<int>& searchable_roles)
    : QVariantListModel(parent) {
  SetRoleNames(role_names);
  this->searchable_roles = searchable_roles;
}

QVariantList SimpleQVariantListModel::GetRow(int i) const { return list[i]; }

int SimpleQVariantListModel::GetRowCount() const { return list.size(); }
