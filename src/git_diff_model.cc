#include "git_diff_model.h"

#include <QClipboard>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QQmlContext>

#include "application.h"
#include "database.h"
#include "theme.h"

#define LOG() qDebug() << "[GitDiffModel]"

GitDiffModel::GitDiffModel(QObject *parent)
    : QAbstractListModel(parent),
      before_line_number_max_width(0),
      after_line_number_max_width(0),
      selected_side(0),
      syntax_formatter(new SyntaxFormatter(this)),
      before_selection_formatter(
          new DiffSelectionFormatter(this, selected_side, 0, selection)),
      after_selection_formatter(
          new DiffSelectionFormatter(this, selected_side, 1, selection)),
      current_chunk(0),
      selected_result(0),
      before_search_formatter(new DiffSearchFormatter(this, 0, search_results,
                                                      search_result_index)),
      after_search_formatter(new DiffSearchFormatter(this, 1, search_results,
                                                     search_result_index)),
      side_by_side_view(true) {
  connect(this, &GitDiffModel::rawDiffChanged, this, &GitDiffModel::ParseDiff);
  connect(this, &GitDiffModel::fileChanged, this,
          [this] { syntax_formatter->DetectLanguageByFile(file); });
  connect(this, &GitDiffModel::isSideBySideViewChanged, this,
          &GitDiffModel::ParseDiff);
  Database::LoadState(this, "SELECT side_by_side_view FROM git_diff_context",
                      {}, [this](QVariantList data) {
                        if (data.isEmpty()) {
                          return;
                        }
                        side_by_side_view = data[0].toBool();
                        LOG() << "Loaded diff view setting. Side-by-side view:"
                              << side_by_side_view;
                        emit isSideBySideViewChanged();
                      });
}

int GitDiffModel::rowCount(const QModelIndex &) const { return lines.size(); }

QVariant GitDiffModel::data(const QModelIndex &index, int role) const {
  const DiffLine &line = lines[index.row()];
  switch (role) {
    case 0:
      return line.before_line_number;
    case 1:
      return raw_diff.sliced(line.before_line.offset, line.before_line.length);
    case 2:
      return line.after_line_number;
    case 3:
      return raw_diff.sliced(line.after_line.offset, line.after_line.length);
    case 4:
      if (line.header.length == 0) {
        return "";
      } else {
        return raw_diff.sliced(line.header.offset, line.header.length);
      }
    case 5:
      return line.is_delete;
    case 6:
      return line.is_add;
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> GitDiffModel::roleNames() const {
  return {
      {0, "beforeLineNumber"},
      {1, "beforeLine"},
      {2, "afterLineNumber"},
      {3, "afterLine"},
      {4, "header"},
      {5, "isDelete"},
      {6, "isAdd"},
  };
}

void GitDiffModel::SetSelectedSide(int side) {
  if (side == selected_side) {
    return;
  }
  selection.Reset([this](int a, int b) { emit rehighlightLines(a, b); });
  selected_side = side;
  emit selectedSideChanged();
}

int GitDiffModel::GetSelectedSide() const { return selected_side; }

int GitDiffModel::GetChunkCount() const { return chunk_offsets.size(); }

QString GitDiffModel::GetSearchResultsCount() const {
  if (search_results.isEmpty()) {
    return "No Results";
  } else {
    return QString::number(selected_result + 1) + " of " +
           QString::number(search_results.size());
  }
}

bool GitDiffModel::AreSearchResultsEmpty() const {
  return search_results.isEmpty();
}

bool GitDiffModel::IsSideBySideView() const {
  return side_by_side_view && !raw_diff.contains("\ndeleted file") &&
         !raw_diff.contains("\nnew file");
}

bool GitDiffModel::resetSelection() {
  return selection.Reset([this](int a, int b) { emit rehighlightLines(a, b); });
}

void GitDiffModel::copySelection(int current_line) const {
  if (lines.isEmpty()) {
    return;
  }
  TextSelection s = selection.Normalize();
  if (s.first_line < 0) {
    s.first_line = s.last_line = current_line;
  }
  QStringList lines_to_copy;
  for (int i = s.first_line; i <= s.last_line; i++) {
    QString txt = GetSelectedTextInLine(s, i);
    if (!txt.isEmpty()) {
      lines_to_copy.append(txt);
    }
  }
  QGuiApplication::clipboard()->setText(lines_to_copy.join('\n'));
}

void GitDiffModel::openFileInEditor(int current_line) const {
  const DiffLine &line = lines[current_line];
  int line_number = line.after_line_number;
  if (line_number < 0) {
    line_number = line.before_line_number;
  }
  LOG() << "Opening git diff chunk at line" << line_number << "of" << file
        << "in editor";
  Application::Get().editor.OpenFile(file, line_number, -1);
}

void GitDiffModel::selectCurrentChunk(int current_line) {
  int i = 0;
  for (; i < chunk_offsets.size(); i++) {
    if (current_line < chunk_offsets[i]) {
      current_chunk = i - 1;
      break;
    }
  }
  emit currentChunkChanged();
}

void GitDiffModel::search(const QString &term) {
  int prev_line = -1;
  int prev_side = -1;
  if (selected_result < search_results.size()) {
    const DiffSearchResult &r = search_results[selected_result];
    prev_line = r.line;
    prev_side = r.side;
  }
  search_results.clear();
  search_result_index.clear();
  if (term.size() > 2) {
    for (int i = 0; i < lines.size(); i++) {
      const DiffLine &line = lines[i];
      QList<TextSegment> sides_to_search = {line.before_line, line.after_line};
      for (int j = 0; j < sides_to_search.size(); j++) {
        TextSegment side = sides_to_search[j];
        QString text = raw_diff.sliced(side.offset, side.length);
        int pos = 0;
        while (true) {
          int k = text.indexOf(term, pos, Qt::CaseInsensitive);
          if (k < 0) {
            break;
          }
          DiffSearchResult result;
          result.offset = k;
          result.length = term.size();
          result.side = j;
          result.line = i;
          search_result_index[i].append(search_results.size());
          search_results.append(result);
          pos = k + term.size();
        }
      }
    }
  }
  if (selected_result >= search_results.size() ||
      search_results[selected_result].line != prev_line ||
      search_results[selected_result].side != prev_side) {
    selected_result = 0;
    for (int i = 0; i < search_results.size(); i++) {
      const DiffSearchResult &r = search_results[i];
      if (r.line >= prev_line && r.side >= prev_side) {
        selected_result = i;
        break;
      }
    }
  }
  emit rehighlightLines(0, lines.size() - 1);
  SelectCurrentSearchResult();
}

void GitDiffModel::goToSearchResult(bool next) {
  if (search_results.isEmpty()) {
    return;
  }
  selected_result += next ? 1 : -1;
  if (selected_result < 0) {
    selected_result = search_results.size() - 1;
  } else if (selected_result >= search_results.size()) {
    selected_result = 0;
  }
  SelectCurrentSearchResult();
}

void GitDiffModel::goToSearchResultInLineAndSide(int line, int side) {
  if (search_results.isEmpty()) {
    return;
  }
  for (int sri : search_result_index[line]) {
    if (search_results[sri].side != side) {
      continue;
    }
    selected_result = sri;
    SelectCurrentSearchResult();
    break;
  }
}

QString GitDiffModel::getSelectedText() const {
  if (selection.first_line < 0 || selection.first_line >= lines.size()) {
    return "";
  }
  TextSelection s = selection.Normalize();
  return GetSelectedTextInLine(s, s.first_line);
}

void GitDiffModel::toggleUnifiedView() {
  side_by_side_view = !side_by_side_view;
  LOG() << "Changing view: side-by-side =" << side_by_side_view;
  emit isSideBySideViewChanged();
  Database::ExecCmdAsync("UPDATE git_diff_context SET side_by_side_view = ?",
                         {side_by_side_view});
}

void GitDiffModel::selectInline(int line, int start, int end) {
  selection.SelectInline(line, start, end,
                         [this](int a, int b) { emit rehighlightLines(a, b); });
}

void GitDiffModel::selectLine(int line) {
  selection.SelectLine(line, lines.size(),
                       [this](int a, int b) { emit rehighlightLines(a, b); });
}

void GitDiffModel::selectAll() {
  selection.SelectAll(lines.size(),
                      [this](int a, int b) { emit rehighlightLines(a, b); });
}

static int ParseLineNumber(const QString &str, QChar start) {
  int i = str.indexOf(start) + 1;
  int j = str.indexOf(',', i);
  if (j < 0) {
    j = str.indexOf(' ', i);
  }
  return str.sliced(i, j - i).toInt();
}

void GitDiffModel::ParseDiff() {
  selected_side = 0;
  selection = TextSelection{};
  chunk_offsets.clear();
  int max_before_line_number = -1, max_after_line_number = -1;
  QList<DiffLine> new_lines;
  if (IsSideBySideView()) {
    new_lines =
        ParseIntoSideBySideDiff(max_before_line_number, max_after_line_number);
  } else {
    new_lines =
        ParseIntoUnifiedDiff(max_before_line_number, max_after_line_number);
  }
  if (!lines.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, lines.size() - 1);
    lines.clear();
    endRemoveRows();
  }
  if (!new_lines.isEmpty()) {
    beginInsertRows(QModelIndex(), 0, new_lines.size() - 1);
    lines = new_lines;
    endInsertRows();
  }
  Application &app = Application::Get();
  QQmlContext *ctx = app.qml_engine.rootContext();
  QString family = ctx->contextProperty("monoFontFamily").toString();
  int size = ctx->contextProperty("monoFontSize").toInt();
  QFontMetrics m(QFont(family, size));
  before_line_number_max_width =
      m.horizontalAdvance(QString::number(max_before_line_number));
  after_line_number_max_width =
      m.horizontalAdvance(QString::number(max_after_line_number));
  emit modelChanged();
  emit goToLine(0);
}

static void WriteBeforeAfterBuffers(QList<DiffLine> &before_buff,
                                    QList<DiffLine> &after_buff,
                                    QList<DiffLine> &new_lines,
                                    TextSegment header) {
  int cnt = std::max(before_buff.size(), after_buff.size());
  for (int i = 0; i < cnt; i++) {
    DiffLine dl;
    if (i < before_buff.size()) {
      const DiffLine &before = before_buff[i];
      dl.is_delete = before.is_delete;
      dl.before_line_number = before.before_line_number;
      dl.before_line = before.before_line;
    }
    if (i < after_buff.size()) {
      const DiffLine &after = after_buff[i];
      dl.is_add = after.is_add;
      dl.after_line_number = after.after_line_number;
      dl.after_line = after.after_line;
    }
    dl.header = header;
    new_lines.append(dl);
  }
  before_buff.clear();
  after_buff.clear();
}

QList<DiffLine> GitDiffModel::ParseIntoSideBySideDiff(
    int &max_before_line_number, int &max_after_line_number) {
  QList<DiffLine> new_lines;
  int pos = 0;
  bool is_header = true;
  TextSegment header;
  QList<DiffLine> before_buff, after_buff;
  while (true) {
    int i = raw_diff.indexOf('\n', pos);
    if (i < 0) {
      break;
    }
    QString line = raw_diff.sliced(pos, i - pos);
    if (line.startsWith("@@")) {
      chunk_offsets.append(new_lines.size());
      is_header = false;
      header.offset = pos;
      header.length = line.size();
      max_before_line_number = ParseLineNumber(line, '-');
      max_after_line_number = ParseLineNumber(line, '+');
    } else if (is_header) {
    } else if (line.startsWith('-')) {
      DiffLine dl;
      dl.is_delete = true;
      dl.before_line_number = max_before_line_number++;
      dl.before_line = TextSegment{pos + 1, (int)line.size() - 1};
      dl.header = header;
      before_buff.append(dl);
    } else if (line.startsWith('+')) {
      DiffLine dl;
      dl.is_add = true;
      dl.after_line_number = max_after_line_number++;
      dl.after_line = TextSegment{pos + 1, (int)line.size() - 1};
      dl.header = header;
      after_buff.append(dl);
    } else if (line.startsWith(' ')) {
      WriteBeforeAfterBuffers(before_buff, after_buff, new_lines, header);
      DiffLine dl;
      dl.header = header;
      dl.before_line_number = max_before_line_number++;
      dl.before_line = TextSegment{pos + 1, (int)line.size() - 1};
      dl.after_line_number = max_after_line_number++;
      dl.after_line = dl.before_line;
      new_lines.append(dl);
    }
    pos = i + 1;
  }
  WriteBeforeAfterBuffers(before_buff, after_buff, new_lines, header);
  return new_lines;
}

QList<DiffLine> GitDiffModel::ParseIntoUnifiedDiff(int &max_before_line_number,
                                                   int &max_after_line_number) {
  QList<DiffLine> new_lines;
  int pos = 0;
  bool is_header = true;
  TextSegment header;
  while (true) {
    int i = raw_diff.indexOf('\n', pos);
    if (i < 0) {
      break;
    }
    QString line = raw_diff.sliced(pos, i - pos);
    if (line.startsWith("@@")) {
      chunk_offsets.append(new_lines.size());
      is_header = false;
      header.offset = pos;
      header.length = line.size();
      max_before_line_number = ParseLineNumber(line, '-');
      max_after_line_number = ParseLineNumber(line, '+');
    } else if (is_header) {
    } else if (line.startsWith('-')) {
      DiffLine dl;
      dl.is_delete = true;
      dl.before_line_number = max_before_line_number++;
      dl.before_line = TextSegment{pos + 1, (int)line.size() - 1};
      dl.header = header;
      new_lines.append(dl);
    } else if (line.startsWith('+')) {
      DiffLine dl;
      dl.is_add = true;
      dl.before_line_number = max_after_line_number++;
      dl.before_line = TextSegment{pos + 1, (int)line.size() - 1};
      dl.header = header;
      new_lines.append(dl);
    } else if (line.startsWith(' ')) {
      DiffLine dl;
      dl.header = header;
      dl.before_line_number = max_before_line_number++;
      dl.before_line = TextSegment{pos + 1, (int)line.size() - 1};
      max_after_line_number++;
      new_lines.append(dl);
    }
    pos = i + 1;
  }
  return new_lines;
}

void GitDiffModel::SelectCurrentSearchResult() {
  emit searchResultsCountChanged();
  if (search_results.isEmpty()) {
    resetSelection();
  } else {
    const DiffSearchResult &result = search_results[selected_result];
    SetSelectedSide(result.side);
    selectInline(result.line, result.offset, result.offset + result.length);
    emit goToLine(result.line);
  }
}

QString GitDiffModel::GetSelectedTextInLine(const TextSelection &s,
                                            int i) const {
  if (i < 0 || i >= lines.size()) {
    return "";
  }
  const DiffLine &dl = lines[i];
  if ((selected_side == 0 && dl.before_line_number < 0) ||
      (selected_side == 1 && dl.after_line_number < 0)) {
    return "";
  }
  TextSegment segment = selected_side == 0 ? dl.before_line : dl.after_line;
  if (i == s.last_line && s.last_line_offset >= 0) {
    segment.length = s.last_line_offset;
  }
  if (i == s.first_line && s.first_line_offset >= 0) {
    segment.offset += s.first_line_offset;
    segment.length -= s.first_line_offset;
  }
  return raw_diff.sliced(segment.offset, segment.length);
}

DiffSelectionFormatter::DiffSelectionFormatter(QObject *parent,
                                               const int &selected_side,
                                               int side,
                                               const TextSelection &selection)
    : TextFormatter(parent),
      selected_side(selected_side),
      side(side),
      formatter(this, selection) {}

QList<TextFormat> DiffSelectionFormatter::Format(const QString &text,
                                                 LineInfo line) const {
  if (side == selected_side) {
    return formatter.Format(text, line);
  } else {
    return {};
  }
}

DiffSearchFormatter::DiffSearchFormatter(QObject *parent, int side,
                                         const QList<DiffSearchResult> &results,
                                         const QHash<int, QList<int> > &index)
    : TextFormatter(parent), side(side), results(results), index(index) {
  static const Theme kTheme;
  format.setBackground(ViewSystem::BrushFromHex(kTheme.kColorSearchResult));
}

QList<TextFormat> DiffSearchFormatter::Format(const QString &,
                                              LineInfo line) const {
  QList<TextFormat> fs;
  for (int sri : index[line.number]) {
    const DiffSearchResult &r = results[sri];
    if (r.side != side) {
      continue;
    }
    TextFormat f;
    f.offset = r.offset;
    f.length = r.length;
    f.format = format;
    fs.append(f);
  }
  return fs;
}
