#include "text_list_model.h"

#include <QtConcurrent>
#include <algorithm>
#include <limits>

#include "theme.h"

#define LOG() qDebug() << "[TextListModel]"

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

TextListModel::TextListModel(QObject* parent) : QAbstractListModel(parent) {}

int TextListModel::rowCount(const QModelIndex&) const { return items.size(); }

QHash<int, QByteArray> TextListModel::roleNames() const { return role_names; }

QVariant TextListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  if (index.row() < 0 || index.row() >= items.size()) {
    return QVariant();
  }
  const TextListItem& item = items[index.row()];
  Q_ASSERT(role >= 0 && role < item.fields.size());
  return item.fields[role];
}

struct HighlightResult {
  int not_matching_chars = 0;
  int match_fragments = 0;
  qsizetype first_match_pos = -1;
  QString result;
  bool matches = false;
};

struct Row {
  int index;
  QVariantList columns;
  QList<HighlightResult> highlights;
  bool matches = false;
};

static HighlightResult HighlightFuzzySubString(const QString& source,
                                               const QString& term,
                                               int min_sub_match_length) {
  // It is a match if every char of 'term' is found in 'source' in the same
  // order as in 'term' e.g. 'term'="cppdevtools" will match
  // 'source'="cpp-dev-tools".
  // However, those found chars should form continuous sub-matches inside
  // 'source' of at least 'min_sub_match_length' consecutive chars each, meaning
  // that given 'min_sub_match_length'=2, 'term'="cdt" won't match
  // 'source="cpp-dev-tools" because there is no continuous "cd" or "dt" in
  // 'source'.
  // However, the user might still be typing the 'term' and might be
  // at 'term'="cppd". There "d" would be a 1 char match and wouldn't match
  // 'source'="cpp-dev-tools". We don't want that to happen because it would
  // lead to ListView not showing any results for a brief moment which would
  // disorient the user. For such cases a 1 char sub match at the very end of
  // the 'term' is allowed and will be considered a match.
  static const QString kHighlightOpen = "<b>";
  static const QString kHighlightClose = "</b>";
  Q_ASSERT(min_sub_match_length > 0);
  HighlightResult result;
  result.result.reserve(source.size() * 1.5);
  int si = 0;
  int ti = 0;
  // Last ti position that pointed to a confirmed sub-match
  int last_ti_match = 0;
  // Current sub-match length
  int match_length = 0;
  if (source.size() >= term.size()) {
    for (; si < source.size(); si++) {
      if (term[ti].toLower() == source[si].toLower()) {
        match_length++;
        if (match_length >= min_sub_match_length) {
          last_ti_match = ti;
        }
        ti++;
        if (match_length == min_sub_match_length) {
          // Current char sequence is 'min_sub_match_length' chars long, thus it
          // is a confirmed sub-match.
          result.result += kHighlightOpen;
          // We didn't put chars from 'source' to 'result' while this sub-match
          // was not confirmed. Need to do it now.
          result.result += source.sliced(si - match_length + 1, match_length);
          result.match_fragments++;
          if (result.first_match_pos < 0) {
            result.first_match_pos = si - match_length + 1;
          }
        }
      } else {
        if (match_length > 0 && match_length < min_sub_match_length) {
          // Last sub-match didn't become confirmed.
          // Rollback 'term' index to last confirmed sub-match position.
          ti = last_ti_match;
          // Rollback 'source' index to the first char of current sub-match,
          // since we need to add it to the 'result' and restart search from the
          // next char.
          si -= match_length;
        } else if (match_length >= min_sub_match_length) {
          // Last char sequence was a sub-match. Need to close it's highlight.
          result.result += kHighlightClose;
        }
        result.not_matching_chars++;
        match_length = 0;
      }
      if (match_length == 0 || match_length > min_sub_match_length) {
        // We are here only if:
        // a) there is no char sequence being matched at all
        // b) there is a char sequence that is already longer than
        // 'min_sub_match_length' chars
        result.result += source[si];
      }
      if (ti == term.size()) {
        // 'term' ended. Need to stop.
        if (match_length >= min_sub_match_length ||
            (match_length > 0 && result.first_match_pos >= 0)) {
          // There was a sub-match, that needs to be closed.
          if (match_length < min_sub_match_length) {
            // An unconfirmed sub-match at the end of 'term' is allowed.
            result.result += kHighlightOpen;
            // We didn't put chars from 'source' to 'result' while this
            // sub-match was not confirmed. Need to do it now.
            result.result += source.sliced(si - match_length + 1, match_length);
            result.match_fragments++;
          }
          result.result += kHighlightClose;
          if (++si < source.size()) {
            // There were more chars in 'source' left after the match.
            // They need to be appended to the 'result'.
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

void TextListModel::Load(int item_to_select) {
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
                row.index = i;
                row.columns = GetRow(i);
                if (filter.size() < min_filter_sub_match_length) {
                  row.matches = true;
                  continue;
                }
                row.highlights = QList<HighlightResult>(row.columns.size());
                for (int role : searchable_roles) {
                  QString str = row.columns[role].toString();
                  HighlightResult h = HighlightFuzzySubString(
                      str, filter, min_filter_sub_match_length);
                  row.highlights[role] = h;
                  if (h.matches) {
                    row.columns[role] = h.result;
                    row.matches = true;
                  }
                }
              }
            });
        // Accumulate a of matches.
        for (int i = first; i <= last; i++) {
          const Row& row = (*not_filtered)[i];
          if (row.matches) {
            filtered->append(row);
          }
        }
      });
  cmd_buffer.ScheduleCommand([this, filtered, item_to_select] {
    // Sort filtered results and schedule updates to UI.
    if (filter.size() >= min_filter_sub_match_length &&
        !searchable_roles.isEmpty()) {
      // Only sort when actual filter is applied. Otherwise - preserve original
      // order, supplied by the client code.
      std::sort(filtered->begin(), filtered->end(),
                [this](const Row& row1, const Row& row2) {
                  return CompareRows(row1, row2, searchable_roles);
                });
    }
    QList<TextListItem> new_items;
    for (const Row& row : *filtered) {
      new_items.append(TextListItem{row.index, row.columns});
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
    cmd_buffer.ScheduleCommand([this, item_to_select] {
      ReSelectItem(item_to_select);
      SetIsUpdating(false);
      emit placeholderChanged();
    });
  });
  cmd_buffer.RunCommands();
}

void TextListModel::LoadNew(int starting_from, int item_to_select) {
  if (starting_from >= GetRowCount()) {
    return;
  }
  if (filter.size() >= min_filter_sub_match_length &&
      !searchable_roles.isEmpty()) {
    Load();
  } else {
    beginInsertRows(QModelIndex(), starting_from, GetRowCount() - 1);
    for (int i = starting_from; i < GetRowCount(); i++) {
      items.append(TextListItem{i, GetRow(i)});
    }
    endInsertRows();
    ReSelectItem(item_to_select);
    emit placeholderChanged();
  }
}

void TextListModel::LoadRemoved(int count) {
  int starting_from = items.size() - count;
  beginRemoveRows(QModelIndex(), starting_from, items.size() - 1);
  items.remove(starting_from, count);
  endRemoveRows();
  emit placeholderChanged();
}

int TextListModel::GetSelectedItemIndex() const { return selected_item_index; }

QString TextListModel::GetPlaceholderText() const {
  if (!placeholder.IsNull()) {
    return placeholder.text;
  } else if (!empty_list_placeholder.IsNull() && GetRowCount() == 0) {
    return empty_list_placeholder.text;
  } else {
    return "";
  }
}

QString TextListModel::GetPlaceholderColor() const {
  static const Theme kTheme;
  QString color = kTheme.kColorText;
  if (!placeholder.IsNull()) {
    color = placeholder.color;
  } else if (!empty_list_placeholder.IsNull() && GetRowCount() == 0) {
    color = empty_list_placeholder.color;
  }
  return color;
}

void TextListModel::SetEmptyListPlaceholder(const QString& text,
                                            const QString& color) {
  empty_list_placeholder = Placeholder(text, color);
  emit placeholderChanged();
}

void TextListModel::SetPlaceholder(const QString& text, const QString& color) {
  this->placeholder = Placeholder(text, color);
  emit placeholderChanged();
}

bool TextListModel::IsUpdating() const { return is_updating; }

QVariant TextListModel::getFieldByRoleName(int row, const QString& name) const {
  if (!name_to_role.contains(name)) {
    return QVariant();
  }
  int role = name_to_role[name];
  if (row < 0 || row >= items.size()) {
    return QVariant();
  }
  const TextListItem& item = items[row];
  if (role < 0 || role >= item.fields.size()) {
    return QVariant();
  }
  return item.fields[role];
}

void TextListModel::selectItemByIndex(int i) {
  int old_index = selected_item_index;
  if (i < 0 || i >= items.size()) {
    selected_item_index = -1;
  } else {
    const TextListItem& item = items[i];
    selected_item_index = item.index;
  }
  if (old_index != selected_item_index) {
    LOG() << "Selected item index changed:" << selected_item_index;
    emit selectedItemChanged();
  }
}

bool TextListModel::SetFilterIfChanged(const QString& filter) {
  if (filter == this->filter) {
    return false;
  }
  return SetFilter(filter);
}

bool TextListModel::SetFilter(const QString& filter) {
  bool should_load = filter.size() >= min_filter_sub_match_length ||
                     (this->filter.size() >= min_filter_sub_match_length &&
                      filter.size() < min_filter_sub_match_length);
  this->filter = filter;
  emit filterChanged();
  if (should_load) {
    Load();
  }
  return should_load;
}

QString TextListModel::GetFilter() { return filter; }

void TextListModel::SetRoleNames(const QHash<int, QByteArray>& role_names) {
  this->role_names = role_names;
  for (auto it = role_names.begin(); it != role_names.end(); it++) {
    QString name(it.value());
    name_to_role[name] = it.key();
  }
}

void TextListModel::SetIsUpdating(bool is_updating) {
  this->is_updating = is_updating;
  emit isUpdatingChanged();
}

void TextListModel::ReSelectItem(int index) {
  int current_index = 0;
  int item_index = index;
  if (item_index < 0) {
    item_index = selected_item_index;
  }
  for (int i = 0; i < items.size(); i++) {
    if (items[i].index == item_index) {
      current_index = i;
      break;
    }
  }
  selectItemByIndex(current_index);
  emit preSelectCurrentIndex(current_index);
}
