#include "text_area_controller.h"

#include <QClipboard>
#include <QFontMetrics>
#include <QGuiApplication>
#include <algorithm>
#include <cmath>
#include <limits>

#include "application.h"
#include "theme.h"

BigTextAreaModel::BigTextAreaModel(QObject* parent)
    : QAbstractListModel(parent),
      cursor_follow_end(false),
      cursor_position(-1),
      selection_formatter(new SelectionFormatter(this, selection)) {
  connect(this, &BigTextAreaModel::cursorPositionChanged, this, [this] {
    if (cursor_position >= 0) {
      emit goToLine(GetLineWithOffset(cursor_position));
    }
  });
}

QHash<int, QByteArray> BigTextAreaModel::roleNames() const {
  return {{0, "text"}, {1, "offset"}};
}

int BigTextAreaModel::rowCount(const QModelIndex&) const {
  return line_start_offsets.size();
}

QVariant BigTextAreaModel::data(const QModelIndex& index, int role) const {
  if (role == 0) {
    int start = line_start_offsets[index.row()];
    int length = GetLineLength(index.row());
    return text.sliced(start, length);
  } else if (role == 1) {
    return line_start_offsets[index.row()];
  } else {
    return QVariant();
  }
}

void BigTextAreaModel::SetText(const QString& text) {
  bool is_append = !this->text.isEmpty() && text.startsWith(this->text);
  int first_new_line = 0;
  if (is_append) {
    first_new_line = line_start_offsets.constLast();
    beginRemoveRows(QModelIndex(), line_start_offsets.size() - 1,
                    line_start_offsets.size() - 1);
    line_start_offsets.removeLast();
    endRemoveRows();
  } else if (!line_start_offsets.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, line_start_offsets.size() - 1);
    line_start_offsets.clear();
    selection = TextSelection();
    endRemoveRows();
  }
  QList<int> new_lines;
  if (!text.isEmpty()) {
    new_lines.append(first_new_line);
  }
  int pos = is_append ? this->text.size() : 0;
  while (true) {
    int i = text.indexOf('\n', pos);
    if (i < 0) {
      break;
    }
    new_lines.append(i + 1);
    pos = new_lines.constLast();
  }
  this->text = text;
  if (new_lines.isEmpty()) {
    return;
  }
  beginInsertRows(QModelIndex(), line_start_offsets.size(),
                  line_start_offsets.size() + new_lines.size() - 1);
  line_start_offsets.append(new_lines);
  endInsertRows();
  emit textChanged();
  if (cursor_follow_end) {
    emit goToLine(line_start_offsets.size() - 1);
  } else if (cursor_position >= 0) {
    emit goToLine(GetLineWithOffset(cursor_position));
  }
}

QString BigTextAreaModel::GetText() const { return text; }

int BigTextAreaModel::GetLineNumberMaxWidth() const {
  QFontMetrics m(font);
  return m.horizontalAdvance(QString::number(line_start_offsets.size()));
}

void BigTextAreaModel::selectInline(int line, int start, int end) {
  selection.SelectInline(line, start, end,
                         [this](int a, int b) { emit rehighlightLines(a, b); });
}

void BigTextAreaModel::selectLine(int line) {
  selection.SelectLine(line, line_start_offsets.size(),
                       [this](int a, int b) { emit rehighlightLines(a, b); });
}

void BigTextAreaModel::selectAll() {
  selection.SelectAll(line_start_offsets.size(),
                      [this](int a, int b) { emit rehighlightLines(a, b); });
}

void BigTextAreaModel::selectSearchResult(int offset, int length) {
  if (length > 0) {
    int line = GetLineWithOffset(offset);
    int line_offset = offset - line_start_offsets[line];
    selectInline(line, line_offset, line_offset + length);
    emit goToLine(line);
  } else {
    resetSelection();
  }
}

bool BigTextAreaModel::resetSelection() {
  return selection.Reset([this](int a, int b) { emit rehighlightLines(a, b); });
}

void BigTextAreaModel::copySelection(int current_line) {
  TextSelection s = selection.Normalize();
  std::pair<int, int> range;
  if (s.first_line < 0) {
    int start = line_start_offsets[current_line];
    range = std::make_pair(start, start + GetLineLength(current_line));
  } else {
    range = GetSelectionRange(s);
  }
  QString text = this->text.sliced(range.first, range.second - range.first);
  QGuiApplication::clipboard()->setText(text);
}

int BigTextAreaModel::getSelectionOffset() {
  TextSelection s = selection.Normalize();
  if (s.first_line < 0) {
    return -1;
  }
  return line_start_offsets[s.first_line] + std::max(s.first_line_offset, 0);
}

QString BigTextAreaModel::getSelectedText() {
  TextSelection s = selection.Normalize();
  if (s.first_line < 0) {
    return "";
  }
  std::pair<int, int> range = GetSelectionRange(s);
  return text.sliced(range.first, range.second - range.first);
}

void BigTextAreaModel::rehighlight() {
  emit rehighlightLines(0, line_start_offsets.size());
}

int BigTextAreaModel::GetLineLength(int line) const {
  if (line < 0 || line >= line_start_offsets.size()) {
    return 0;
  }
  int start = line_start_offsets[line];
  int end = line < line_start_offsets.size() - 1
                ? line_start_offsets[line + 1] - 1
                : text.size();
  return std::max(end - start, 0);
}

int BigTextAreaModel::GetLineWithOffset(int offset) const {
  for (int i = 0; i < line_start_offsets.size(); i++) {
    if (offset < line_start_offsets[i]) {
      return i - 1;
    }
  }
  return 0;
}

LineHighlighter::LineHighlighter(QObject* parent)
    : QSyntaxHighlighter(parent), document(nullptr), line_number(0) {}

QQuickTextDocument* LineHighlighter::GetDocument() const { return document; }

void LineHighlighter::SetDocument(QQuickTextDocument* document) {
  this->document = document;
  setDocument(document->textDocument());
  emit documentChanged();
}

void LineHighlighter::highlightBlock(const QString& text) {
  LineInfo line{line_offset, line_number};
  for (const TextFormatter* formatter : formatters) {
    for (const TextFormat& f : formatter->Format(text, line)) {
      setFormat(f.offset, f.length, f.format);
    }
  }
}

SelectionFormatter::SelectionFormatter(QObject* parent,
                                       const TextSelection& selection)
    : TextFormatter(parent), selection(selection) {
  static const Theme kTheme;
  format.setForeground(ViewSystem::BrushFromHex(kTheme.kColorText));
  format.setBackground(ViewSystem::BrushFromHex(kTheme.kColorTextSelection));
}

QList<TextFormat> SelectionFormatter::Format(const QString& text,
                                             LineInfo line) const {
  TextSelection s = selection.Normalize();
  if (line.number < s.first_line || line.number > s.last_line) {
    return {};
  }
  TextFormat f;
  f.offset = std::max(s.first_line_offset, 0);
  int end = s.last_line_offset;
  if (end < 0) {
    end = text.size();
  }
  f.length = end - f.offset;
  f.format = format;
  return {f};
}

TextFormatter::TextFormatter(QObject* parent) : QObject(parent) {}

TextSelection::TextSelection()
    : first_line(-1),
      first_line_offset(-1),
      last_line(-1),
      last_line_offset(-1),
      multiline_selection(false) {}

std::pair<int, int> TextSelection::GetLineRange() const {
  int first = std::max(first_line, 0);
  int last = std::max(last_line, 0);
  if (first > last) {
    return std::make_pair(last, first);
  } else {
    return std::make_pair(first, last);
  }
}

TextSelection TextSelection::Normalize() const {
  TextSelection result = *this;
  if (first_line > last_line) {
    result.first_line = last_line;
    result.first_line_offset = last_line_offset;
    result.last_line = first_line;
    result.last_line_offset = first_line_offset;
  }
  return result;
}

void TextSelection::SelectInline(
    int line, int start, int end,
    const std::function<void(int, int)>& rehighlight) {
  std::pair<int, int> redraw_range = GetLineRange();
  multiline_selection = false;
  last_line = first_line = line;
  first_line_offset = start;
  last_line_offset = end;
  rehighlight(redraw_range.first, redraw_range.second);
  rehighlight(line, line);
}

void TextSelection::SelectLine(
    int line, int line_count,
    const std::function<void(int, int)>& rehighlight) {
  if (line < 0 || line >= line_count) {
    return;
  }
  if (!multiline_selection) {
    std::pair<int, int> redraw_range = GetLineRange();
    multiline_selection = true;
    first_line = last_line = line;
    first_line_offset = last_line_offset = -1;
    rehighlight(redraw_range.first, redraw_range.second);
    rehighlight(line, line);
  } else {
    std::pair<int, int> redraw_range = GetLineRange();
    last_line = line;
    rehighlight(redraw_range.first, redraw_range.second);
    redraw_range = GetLineRange();
    rehighlight(redraw_range.first, redraw_range.second);
  }
}

void TextSelection::SelectAll(
    int line_count, const std::function<void(int, int)>& rehighlight) {
  std::pair<int, int> redraw_range = GetLineRange();
  *this = TextSelection();
  first_line = 0;
  last_line = std::max(line_count - 1, 0);
  rehighlight(redraw_range.first, redraw_range.second);
  redraw_range = GetLineRange();
  rehighlight(redraw_range.first, redraw_range.second);
}

bool TextSelection::Reset(const std::function<void(int, int)>& rehighlight) {
  if (first_line < 0) {
    return false;
  }
  std::pair<int, int> redraw_range = GetLineRange();
  *this = TextSelection();
  rehighlight(redraw_range.first, redraw_range.second);
  return true;
}

std::pair<int, int> BigTextAreaModel::GetSelectionRange(
    const TextSelection& s) const {
  int start = line_start_offsets[s.first_line];
  int end = line_start_offsets[s.last_line];
  if (s.first_line_offset >= 0) {
    start += s.first_line_offset;
  }
  if (s.last_line_offset >= 0) {
    end += s.last_line_offset;
  } else {
    end += GetLineLength(s.last_line);
  }
  return std::make_pair(start, end);
}

TextSearchController::TextSearchController(QObject* parent)
    : QObject(parent), formatter(new SearchFormatter(this, results, index)) {}

bool TextSearchController::AreSearchResultsEmpty() const {
  return results.isEmpty();
}

QString TextSearchController::GetSearchResultsCount() const {
  if (results.isEmpty()) {
    return "No Results";
  } else {
    return QString::number(selected_result + 1) + " of " +
           QString::number(results.size());
  }
}

void TextSearchController::search(const QString& term, const QString& text,
                                  bool select_result,
                                  bool notify_results_changed) {
  int prev_start = -1;
  if (selected_result < results.size()) {
    prev_start = results[selected_result].offset;
  }
  results.clear();
  index.clear();
  if (term.size() > 2) {
    int line = 0;
    int pos = 0;
    while (true) {
      int i = text.indexOf(term, pos, Qt::CaseSensitivity::CaseInsensitive);
      if (i < 0) {
        break;
      }
      line += text.sliced(pos, i - pos).count('\n');
      index[line].append(results.size());
      TextSegment result;
      result.offset = i;
      result.length = term.size();
      results.append(result);
      pos = result.offset + result.length;
    }
  }
  if (notify_results_changed) {
    emit searchResultsChanged();
  }
  if (selected_result >= results.size() ||
      results[selected_result].offset != prev_start) {
    selected_result = 0;
    for (int i = 0; i < results.size(); i++) {
      if (results[i].offset >= prev_start) {
        selected_result = i;
        break;
      }
    }
  }
  if (select_result) {
    if (!results.isEmpty()) {
      DisplaySelectedSearchResult();
    } else {
      emit selectResult(0, 0);
    }
  }
  emit searchResultsCountChanged();
}

void TextSearchController::replaceSearchResultWith(const QString& text,
                                                   bool replace_all) {
  if (results.isEmpty()) {
    return;
  }
  QList<int> offsets, lengths;
  if (replace_all) {
    for (auto it = results.rbegin(); it != results.rend(); it++) {
      offsets.append(it->offset);
      lengths.append(it->length);
    }
  } else {
    TextSegment result = results[selected_result];
    offsets.append(result.offset);
    lengths.append(result.length);
  }
  emit replaceResults(offsets, lengths, text);
}

void TextSearchController::goToResultWithStartAt(int text_position) {
  for (int i = 0; i < results.size(); i++) {
    if (results[i].offset == text_position) {
      selected_result = i;
      DisplaySelectedSearchResult();
      emit searchResultsCountChanged();
      break;
    }
  }
}

void TextSearchController::goToSearchResult(bool next) {
  if (results.isEmpty()) {
    return;
  }
  selected_result = next ? selected_result + 1 : selected_result - 1;
  if (selected_result >= results.size()) {
    selected_result = 0;
  } else if (selected_result < 0) {
    selected_result = results.size() - 1;
  }
  DisplaySelectedSearchResult();
  emit searchResultsCountChanged();
}

void TextSearchController::DisplaySelectedSearchResult() {
  TextSegment result = results[selected_result];
  emit selectResult(result.offset, result.length);
}

SearchFormatter::SearchFormatter(QObject* parent,
                                 const QList<TextSegment>& results,
                                 const QHash<int, QList<int>>& index)
    : TextFormatter(parent), results(results), index(index) {
  static const Theme kTheme;
  format.setBackground(ViewSystem::BrushFromHex(kTheme.kColorSearchResult));
}

QList<TextFormat> SearchFormatter::Format(const QString&, LineInfo line) const {
  QList<TextFormat> fs;
  const QList<int>& sris = index[line.number];
  for (int sri : sris) {
    TextSegment sr = results[sri];
    TextFormat f;
    f.offset = sr.offset - line.offset;
    f.length = sr.length;
    f.format = format;
    fs.append(f);
  }
  return fs;
}

SmallTextAreaHighlighter::SmallTextAreaHighlighter(QObject* parent)
    : QSyntaxHighlighter(parent), document(nullptr) {}

QQuickTextDocument* SmallTextAreaHighlighter::GetDocument() const {
  return document;
}

void SmallTextAreaHighlighter::SetDocument(QQuickTextDocument* document) {
  this->document = document;
  setDocument(document->textDocument());
  emit documentChanged();
}

void SmallTextAreaHighlighter::highlightBlock(const QString& text) {
  QTextBlock block = currentBlock();
  LineInfo line{block.position(), block.firstLineNumber()};
  for (const TextFormatter* formatter : formatters) {
    for (const TextFormat& f : formatter->Format(text, line)) {
      setFormat(f.offset, f.length, f.format);
    }
  }
}

DummyFormatter::DummyFormatter(QObject* parent) : TextFormatter(parent) {}

QList<TextFormat> DummyFormatter::Format(const QString&, LineInfo) const {
  return {};
}

FileLinkFormatter::FileLinkFormatter(QObject* parent,
                                     const QList<FileLink>& links,
                                     const QHash<int, QList<int>>& index,
                                     const int& current_line,
                                     const int& current_line_link)
    : TextFormatter(parent),
      links(links),
      index(index),
      current_line(current_line),
      current_line_link(current_line_link) {
  static const Theme kTheme;
  format.setForeground(ViewSystem::BrushFromHex(kTheme.kColorPrimary));
}

QList<TextFormat> FileLinkFormatter::Format(const QString&,
                                            LineInfo line) const {
  QList<TextFormat> fs;
  const QList<int>& flis = index[line.number];
  for (int i = 0; i < flis.size(); i++) {
    int fli = flis[i];
    const FileLink& fl = links[fli];
    TextFormat f;
    f.offset = fl.offset - line.offset;
    f.length = fl.length;
    f.format = format;
    if (current_line == line.number && i == current_line_link) {
      f.format.setFontUnderline(true);
    }
    fs.append(f);
  }
  return fs;
}

FileLinkLookupController::FileLinkLookupController(QObject* parent)
    : QObject(parent),
      current_line(0),
      current_line_link(0),
      formatter(new FileLinkFormatter(this, links, index, current_line,
                                      current_line_link)) {}

void FileLinkLookupController::findFileLinks(const QString& text) {
  static const QRegularExpression kUnix1(
      "([A-Z]?\\:?\\/[^:\\n]+):([0-9]+):?([0-9]+)?");
  static const QRegularExpression kUnix2(
      "([A-Z]?\\:?\\/[^:\\n]+)\\(([0-9]+):?([0-9]+)?\\)");
  static const QRegularExpression kWin1(
      "([A-Z]\\:\\\\[^:\\n]+)\\(([0-9]+),?([0-9]+)?\\)");
  static const QRegularExpression kWin2(
      "([A-Z]\\:\\\\[^:\\n]+):([0-9]+):([0-9]+)?");
  links.clear();
  index.clear();
  FindFileLinks(kUnix1, text);
  FindFileLinks(kUnix2, text);
  FindFileLinks(kWin1, text);
  FindFileLinks(kWin2, text);
}

void FileLinkLookupController::setCurrentLine(int line) {
  if (line == current_line) {
    // We can only get here if we ourselves initiated change of the line. In
    // such case we don't need to do anything because we must've already
    // properly set and rehighlighted everything.
    return;
  }
  int old = current_line;
  current_line = line;
  current_line_link = 0;
  emit rehighlightLine(old);
  emit rehighlightLine(line);
}

void FileLinkLookupController::openCurrentFileLink() {
  if (!index.contains(current_line)) {
    return;
  }
  int i = index[current_line][current_line_link];
  const FileLink& link = links[i];
  Application::Get().editor.OpenFile(link.file_path, link.line_num,
                                     link.col_num);
}

void FileLinkLookupController::goToLink(bool next) {
  int i;
  for (i = 0; i < links.size() && links[i].line < current_line; i++) {
  }
  i += current_line_link;
  i += next ? 1 : -1;
  if (i < 0 || i >= links.size()) {
    return;
  }
  int old = current_line;
  current_line = links[i].line;
  current_line_link = index[current_line].indexOf(i);
  emit rehighlightLine(old);
  emit rehighlightLine(current_line);
  emit linkInLineSelected(current_line);
}

void FileLinkLookupController::FindFileLinks(const QRegularExpression& regex,
                                             const QString& text) {
  int line = 0;
  int pos = 0;
  for (auto it = regex.globalMatch(text); it.hasNext();) {
    QRegularExpressionMatch m = it.next();
    FileLink link;
    link.file_path = m.captured(1);
    link.offset = m.capturedStart();
    link.length = m.capturedLength();
    link.line_num = m.captured(2).toInt();
    if (m.lastCapturedIndex() > 2) {
      link.col_num = m.captured(3).toInt();
    }
    line += text.sliced(pos, link.offset - pos).count('\n');
    link.line = line;
    pos = link.offset;
    index[line].append(links.size());
    links.append(link);
  }
}
