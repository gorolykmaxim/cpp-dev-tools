#include "qvariant_list_model.h"

#include <algorithm>
#include <limits>

UiCommandBuffer::UiCommandBuffer() : QObject() {
  QObject::connect(this, &UiCommandBuffer::commandsReady, this,
                   &UiCommandBuffer::ExecuteCommand, Qt::QueuedConnection);
}

void UiCommandBuffer::ScheduleCommands(
    int items, int items_per_cmd, const std::function<void(int, int)>& cmd) {
  int cmd_count = items / items_per_cmd;
  for (int i = 0; i <= cmd_count; i++) {
    int first = i * items_per_cmd;
    int last;
    if (i < cmd_count) {
      last = (i + 1) * items_per_cmd - 1;
    } else if (items - 1 >= first) {
      last = items - 1;
    } else {
      break;
    }
    commands.enqueue([cmd, first, last] { cmd(first, last); });
  }
}

void UiCommandBuffer::ScheduleCommand(const std::function<void()>& cmd) {
  commands.enqueue(cmd);
}

void UiCommandBuffer::RunCommands() { emit commandsReady(); }

void UiCommandBuffer::Clear() { commands.clear(); }

void UiCommandBuffer::ExecuteCommand() {
  if (commands.isEmpty()) {
    return;
  }
  std::function<void()> cmd = commands.dequeue();
  cmd();
  emit commandsReady();
}

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
  int match_fragments = 0;
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
    result.match_fragments++;
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
    result.match_fragments = std::numeric_limits<int>::max();
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
    if (props1.match_fragments != props2.match_fragments) {
      return props1.match_fragments < props2.match_fragments;
    } else if (props1.first_match_pos != props2.first_match_pos) {
      return props1.first_match_pos < props2.first_match_pos;
    } else if (props1.not_matching_chars != props2.not_matching_chars) {
      return props1.not_matching_chars < props2.not_matching_chars;
    }
  }
  return false;
}

void QVariantListModel::Load() {
  SetIsUpdating(true);
  QList<QVariantList> new_items;
  int count = GetRowCount();
  for (int i = 0; i < count; i++) {
    QVariantList row = GetRow(i);
    if (filter.isEmpty() || searchable_roles.isEmpty()) {
      new_items.append(row);
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
      new_items.append(row);
    }
  }
  if (!filter.isEmpty()) {
    // Only sort when actual filter is applied. Otherwise - preserve original
    // order, supplied by the client code.
    std::sort(new_items.begin(), new_items.end(),
              [this](const QVariantList& row1, const QVariantList& row2) {
                return CompareRows(row1, row2, searchable_roles);
              });
  }
  int diff = new_items.size() - items.size();
  cmd_buffer.Clear();
  if (diff > 0) {
    cmd_buffer.ScheduleCommands(
        diff, 100, [this, new_items](int first, int last) {
          int to_insert = last - first + 1;
          beginInsertRows(QModelIndex(), items.size(),
                          items.size() + to_insert - 1);
          items.append(new_items.sliced(items.size(), to_insert));
          endInsertRows();
        });
  } else if (diff < 0) {
    cmd_buffer.ScheduleCommands(diff * -1, 100, [this](int first, int last) {
      int to_remove = last - first + 1;
      int starting_from = items.size() - to_remove;
      beginRemoveRows(QModelIndex(), starting_from, items.size() - 1);
      items.remove(starting_from, to_remove);
      endRemoveRows();
    });
  }
  cmd_buffer.ScheduleCommands(std::min(new_items.size(), items.size()), 5,
                              [this, new_items](int first, int last) {
                                for (int i = first; i <= last; i++) {
                                  items[i] = new_items[i];
                                }
                                emit dataChanged(index(first), index(last));
                              });
  cmd_buffer.ScheduleCommand([this] {
    int current_index = 0;
    if (name_to_role.contains("isSelected")) {
      int role = name_to_role["isSelected"];
      for (int i = 0; i < items.size(); i++) {
        if (items[i][role].toBool()) {
          current_index = i;
          break;
        }
      }
    }
    emit preSelectCurrentIndex(current_index);
    SetIsUpdating(false);
  });
  cmd_buffer.RunCommands();
}

QVariant QVariantListModel::getFieldByRoleName(int row,
                                               const QString& name) const {
  if (!name_to_role.contains(name)) {
    return QVariant();
  }
  int role = name_to_role[name];
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

void QVariantListModel::SetIsUpdating(bool is_updating) {
  this->is_updating = is_updating;
  emit isUpdatingChanged();
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
