#include "qvariant_list_model.h"

#include <QtConcurrent>
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

struct HighlightResult {
  int not_matching_chars = 0;
  int match_fragments = 0;
  qsizetype first_match_pos = -1;
  QString result;
  bool matches = false;
};

struct Row {
  QVariantList columns;
  QList<HighlightResult> highlights;
  bool matches = false;
};

static HighlightResult HighlightFuzzySubString(const QString& source,
                                               const QString& term) {
  // It is a match if every char of 'term' is found in 'source'
  // in the same order as in 'term' e.g. 'term'="cppdevtools"
  // will match 'source'="cpp-dev-tools".
  // However, those found chars should form continuous sub-matches
  // inside 'source' of at least 2 consecutive chars each, meaning
  // that 'term'="cdt" won't match 'source="cpp-dev-tools" because
  // there is no continuos "cd" or "dt" in 'source'.
  // However, the user might still be typing the 'term' and might
  // be at 'term'="cppd". There "d" would be a 1 char match and
  // wouldn't match 'source'="cpp-dev-tools". We don't want that
  // to happen because it would lead to ListView not showing
  // any results for a brief moment which would disorient the user.
  // For such cases a 1 char sub match at the very end of
  // the 'term' is allowed and will be considered a match.
  static const QString kHighlightOpen = "<b>";
  static const QString kHighlightClose = "</b>";
  HighlightResult result;
  result.result.reserve(source.size() * 1.5);
  int si = 0;
  int ti = 0;
  int match_length = 0;  // Current sub-match length
  if (source.size() > term.size()) {
    for (; si < source.size(); si++) {
      if (term[ti].toLower() == source[si].toLower()) {
        match_length++;
        ti++;
        if (match_length == 2) {
          // Current char sequence is longer than 1, thus it is a sub-match.
          result.result += kHighlightOpen;
          // We didn't put char from source while sequence was 1 char long.
          // Need to put that char in result now.
          result.result += source[si - 1];
          result.match_fragments++;
          if (result.first_match_pos < 0) {
            result.first_match_pos = si - 1;
          }
        }
      } else {
        result.not_matching_chars++;
        if (match_length == 1) {
          // Last char sequence ended while being 1 char-long.
          // Need to count that char as not matching and rollback
          // 'term' index.
          result.not_matching_chars++;
          ti--;
          // We didn't put char from source while sequence was 1 char long.
          // Need to put that char in result now.
          result.result += source[si - 1];
        } else if (match_length > 1) {
          // Last char sequence was a sub-match. Need to close it's highlight.
          result.result += kHighlightClose;
        }
        match_length = 0;
      }
      if (match_length != 1) {
        // We are here only if:
        // a) there is no char sequence being matched at all
        // b) there is a char sequence that is already longer than 2 chars
        result.result += source[si];
      }
      if (ti == term.size()) {
        // 'term' ended. Need to stop.
        if (match_length > 1 ||
            (match_length == 1 && result.first_match_pos >= 0)) {
          // There was an active sub-match, that needs to be closed.
          if (match_length == 1) {
            // A 1 char sub-match at the end of 'term' is allowed.
            result.result += kHighlightOpen;
            // We are at 1 char sub-match right now, so current
            // 'source' char hasn't been putted in result yet.
            result.result += source[si];
            result.match_fragments++;
          }
          result.result += kHighlightClose;
          if (++si < source.size()) {
            // There were more chars in 'source' left after the match.
            // They need to be appended to the result.
            result.result += source.sliced(si, source.size() - si);
            result.not_matching_chars += source.size() - si;
          }
          result.matches = true;
        }
        break;
      }
    }
  }
  if (!result.matches) {
    result.first_match_pos = std::numeric_limits<qsizetype>::max();
    result.not_matching_chars = std::numeric_limits<int>::max();
    result.match_fragments = std::numeric_limits<int>::max();
    result.result = source;
  }
  return result;
}

static bool CompareRows(const Row& row1, const Row& row2,
                        const QList<int>& searchable_roles) {
  for (int role : searchable_roles) {
    const HighlightResult& highlight1 = row1.highlights[role];
    const HighlightResult& highlight2 = row2.highlights[role];
    if (highlight1.match_fragments != highlight2.match_fragments) {
      return highlight1.match_fragments < highlight2.match_fragments;
    } else if (highlight1.first_match_pos != highlight2.first_match_pos) {
      return highlight1.first_match_pos < highlight2.first_match_pos;
    } else if (highlight1.not_matching_chars != highlight2.not_matching_chars) {
      return highlight1.not_matching_chars < highlight2.not_matching_chars;
    }
  }
  return false;
}

void QVariantListModel::Load() {
  SetIsUpdating(true);
  int count = GetRowCount();
  auto not_filtered = QSharedPointer<QList<Row>>::create(count);
  auto filtered = QSharedPointer<QList<Row>>::create();
  filtered->reserve(count);
  cmd_buffer.Clear();
  cmd_buffer.ScheduleCommands(
      count, 2000, [this, not_filtered, filtered](int first, int last) {
        // Split current items segment among threads.
        int threads = QThreadPool::globalInstance()->maxThreadCount();
        int count_per_thread = (last - first + 1) / threads;
        QList<std::pair<int, int>> ranges;
        int start = first;
        int end = first;
        for (int i = 0; i < threads - 1; i++) {
          start = first + i * count_per_thread;
          end = start + count_per_thread;
          ranges.append(std::make_pair(start, end));
        }
        start = end;
        end = last + 1;
        ranges.append(std::make_pair(start, end));
        // Fuzzy-highlight search results in parallel.
        QtConcurrent::blockingMap(
            ranges, [this, not_filtered](std::pair<int, int> range) {
              for (int i = range.first; i < range.second; i++) {
                Row& row = (*not_filtered)[i];
                row.columns = GetRow(i);
                if (filter.size() < 2) {
                  row.matches = true;
                  continue;
                }
                row.highlights = QList<HighlightResult>(row.columns.size());
                for (int role : searchable_roles) {
                  QString str = row.columns[role].toString();
                  HighlightResult h = HighlightFuzzySubString(str, filter);
                  row.highlights[role] = h;
                  if (h.matches) {
                    row.columns[role] = h.result;
                    row.matches = true;
                  }
                }
              }
            });
        // Accumulate a of matches.
        for (const Row& row : *not_filtered) {
          if (row.matches) {
            filtered->append(row);
          }
        }
      });
  cmd_buffer.ScheduleCommand([this, filtered] {
    // Sort filtered results and schedule updates to UI.
    if (filter.size() > 1 && !searchable_roles.isEmpty()) {
      // Only sort when actual filter is applied. Otherwise - preserve original
      // order, supplied by the client code.
      std::sort(filtered->begin(), filtered->end(),
                [this](const Row& row1, const Row& row2) {
                  return CompareRows(row1, row2, searchable_roles);
                });
    }
    QList<QVariantList> new_items;
    for (const Row& row : *filtered) {
      new_items.append(row.columns);
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
  bool should_load =
      filter.size() > 1 || (this->filter.size() > 1 && filter.size() < 2);
  this->filter = filter;
  emit filterChanged();
  if (should_load) {
    Load();
  }
}

QString QVariantListModel::GetFilter() { return filter; }

QVariantList QVariantListModel::GetRow(int) const { return {}; }

int QVariantListModel::GetRowCount() const { return 0; }

void QVariantListModel::SetRoleNames(const QHash<int, QByteArray>& role_names) {
  this->role_names = role_names;
  for (auto it = role_names.begin(); it != role_names.end(); it++) {
    QString name(it.value());
    name_to_role[name] = it.key();
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
