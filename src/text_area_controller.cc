#include "text_area_controller.h"

#include <QClipboard>
#include <QFontMetrics>
#include <QGuiApplication>
#include <algorithm>
#include <cmath>
#include <limits>

#include "application.h"
#include "theme.h"

static const int kCursorHistoryLimit = 100;
static const int kLinesPerPage = 20;

TextAreaController::TextAreaController(QObject* parent)
    : QObject(parent), highlighter(file_links) {
  cursor_history.reserve(kCursorHistoryLimit);
  UpdateSearchResultsCount();
}

void TextAreaController::search(const QString& term, const QString& text,
                                bool select_result) {
  int prev_start = -1;
  if (selected_result < search_results.size()) {
    prev_start = search_results[selected_result].start;
  }
  search_results.clear();
  if (term.size() > 2) {
    int pos = 0;
    while (true) {
      int i = text.indexOf(term, pos, Qt::CaseSensitivity::CaseInsensitive);
      if (i < 0) {
        break;
      }
      TextSection result;
      result.start = i;
      result.end = i + term.size();
      search_results.append(result);
      pos = result.end;
    }
  }
  if (selected_result >= search_results.size() ||
      search_results[selected_result].start != prev_start) {
    selected_result = 0;
    for (int i = 0; i < search_results.size(); i++) {
      if (search_results[i].start >= prev_start) {
        selected_result = i;
        break;
      }
    }
  }
  if (select_result) {
    if (!search_results.isEmpty()) {
      DisplaySelectedSearchResult();
    } else {
      emit selectText(0, 0);
    }
  }
  UpdateSearchResultsCount();
}

void TextAreaController::replaceSearchResultWith(const QString& text,
                                                 bool replace_all) {
  if (search_results.isEmpty()) {
    return;
  }
  if (replace_all) {
    for (auto it = search_results.rbegin(); it != search_results.rend(); it++) {
      emit replaceText(it->start, it->end, text);
    }
  } else {
    TextSection result = search_results[selected_result];
    emit replaceText(result.start, result.end, text);
  }
}

void TextAreaController::goToResultWithStartAt(int text_position) {
  for (int i = 0; i < search_results.size(); i++) {
    if (search_results[i].start == text_position) {
      selected_result = i;
      DisplaySelectedSearchResult();
      UpdateSearchResultsCount();
      break;
    }
  }
}

void TextAreaController::goToSearchResult(bool next) {
  if (search_results.isEmpty()) {
    return;
  }
  selected_result = next ? selected_result + 1 : selected_result - 1;
  if (selected_result >= search_results.size()) {
    selected_result = 0;
  } else if (selected_result < 0) {
    selected_result = search_results.size() - 1;
  }
  DisplaySelectedSearchResult();
  UpdateSearchResultsCount();
}

void TextAreaController::saveCursorPosition(int position) {
  if (!cursor_history.isEmpty() &&
      cursor_history[cursor_history_index] == position) {
    return;
  }
  if (cursor_history_index != cursor_history.size() - 1) {
    int cnt_to_remove = cursor_history.size() - cursor_history_index - 1;
    cursor_history.remove(cursor_history_index + 1, cnt_to_remove);
  }
  cursor_history.append(position);
  if (cursor_history.size() > kCursorHistoryLimit) {
    cursor_history.removeFirst();
  }
  cursor_history_index = cursor_history.size() - 1;
  emit cursorPositionChanged();
}

void TextAreaController::goToCursorPosition(bool next) {
  int new_index = next ? cursor_history_index + 1 : cursor_history_index - 1;
  if (new_index < 0 || new_index >= cursor_history.size()) {
    return;
  }
  cursor_history_index = new_index;
  int pos = cursor_history[cursor_history_index];
  emit changeCursorPosition(pos);
}

void TextAreaController::resetCursorPositionHistory() {
  cursor_history.clear();
  cursor_history_index = -1;
}

void TextAreaController::findFileLinks(const QString& text) {
  if (!detect_file_links) {
    return;
  }
  static const QRegularExpression kUnix(
      "([A-Z]?\\:?\\/[^:\\n]+):([0-9]+):?([0-9]+)?");
  static const QRegularExpression kWin1(
      "([A-Z]\\:\\\\[^:\\n]+)\\(([0-9]+),?([0-9]+)?\\)");
  static const QRegularExpression kWin2(
      "([A-Z]\\:\\\\[^:\\n]+):([0-9]+):([0-9]+)?");
  file_links.clear();
  FindFileLinks(kUnix, text);
  FindFileLinks(kWin1, text);
  FindFileLinks(kWin2, text);
}

void TextAreaController::openFileLinkAtCursor() {
  if (cursor_history_index < 0) {
    return;
  }
  int pos = cursor_history[cursor_history_index];
  int i = IndexOfFileLinkAtPosition(pos);
  if (i < 0) {
    return;
  }
  const FileLink& link = file_links[i];
  Application::Get().editor.OpenFile(link.file_path, link.column, link.row);
}

void TextAreaController::goToFileLink(bool next) {
  if (cursor_history_index < 0) {
    return;
  }
  int pos = cursor_history[cursor_history_index];
  int i = -1;
  int min_distance = std::numeric_limits<int>::max();
  for (int j = 0; j < file_links.size(); j++) {
    TextSection section = file_links[j].section;
    int distance = std::abs(section.start - pos);
    if (((next && section.start > pos) || (!next && section.start < pos)) &&
        distance < min_distance) {
      i = j;
      min_distance = distance;
    }
  }
  if (i < 0) {
    return;
  }
  pos = file_links[i].section.start;
  saveCursorPosition(pos);
  emit changeCursorPosition(pos);
}

void TextAreaController::rehighlightBlockByLineNumber(int i) {
  if (!document) {
    return;
  }
  QTextDocument* doc = document->textDocument();
  QTextBlock block = doc->findBlockByLineNumber(i);
  highlighter.rehighlightBlock(block);
}

void TextAreaController::goToPage(const QString& text, bool up) {
  int pos = 0;
  if (cursor_history_index >= 0) {
    pos = cursor_history[cursor_history_index];
    // TextArea sometimes tends to put cursor outside the text boundaries.
    pos = std::min(pos, (int)text.size() - 1);
  }
  int line = 0;
  while (!(up && pos <= 0) && !(!up && pos >= text.size() - 1)) {
    if (text[pos] == '\n') {
      line++;
    }
    if (line == kLinesPerPage) {
      break;
    }
    if (up) {
      pos--;
    } else {
      pos++;
    }
  }
  emit changeCursorPosition(pos);
}

bool TextAreaController::AreSearchResultsEmpty() const {
  return search_results.isEmpty();
}

QQuickTextDocument* TextAreaController::GetDocument() { return document; }

void TextAreaController::SetDocument(QQuickTextDocument* document) {
  this->document = document;
  highlighter.setDocument(document->textDocument());
  emit documentChanged();
}

TextAreaFormatter* TextAreaController::GetFormatter() {
  return highlighter.formatter;
}

void TextAreaController::SetFormatter(TextAreaFormatter* formatter) {
  highlighter.formatter = formatter;
  highlighter.rehighlight();
  emit formatterChanged();
}

bool TextAreaController::IsCursorOnLink() const {
  if (cursor_history_index < 0) {
    return false;
  }
  int pos = cursor_history[cursor_history_index];
  return IndexOfFileLinkAtPosition(pos) >= 0;
}

void TextAreaController::UpdateSearchResultsCount() {
  if (!search_results.isEmpty()) {
    search_results_count = QString::number(selected_result + 1) + " of " +
                           QString::number(search_results.size());
  } else {
    search_results_count = "No Results";
  }
  emit searchResultsCountChanged();
}

void TextAreaController::DisplaySelectedSearchResult() {
  const TextSection& result = search_results[selected_result];
  emit selectText(result.start, result.end);
}

void TextAreaController::FindFileLinks(const QRegularExpression& regex,
                                       const QString& text) {
  for (auto it = regex.globalMatch(text); it.hasNext();) {
    QRegularExpressionMatch m = it.next();
    FileLink link;
    link.file_path = m.captured(1);
    link.section.start = m.capturedStart();
    link.section.end = m.capturedEnd();
    link.column = m.captured(2).toInt();
    if (m.lastCapturedIndex() > 2) {
      link.row = m.captured(3).toInt();
    }
    file_links.append(link);
  }
}

int TextAreaController::IndexOfFileLinkAtPosition(int position) const {
  for (int i = 0; i < file_links.size(); i++) {
    TextSection section = file_links[i].section;
    if (position >= section.start && position <= section.end) {
      return i;
    }
  }
  return -1;
}

TextAreaFormatter::TextAreaFormatter(QObject* parent) : QObject(parent) {}

TextAreaHighlighter::TextAreaHighlighter(QList<FileLink>& file_links)
    : QSyntaxHighlighter((QObject*)nullptr), file_links(file_links) {
  Theme theme;
  link_format.setForeground(ViewSystem::BrushFromHex(theme.kColorHighlight));
  link_format.setFontUnderline(true);
}

void TextAreaHighlighter::highlightBlock(const QString& text) {
  QTextBlock block = currentBlock();
  if (formatter) {
    for (const TextSectionFormat& f : formatter->Format(text, block)) {
      setFormat(f.section.start, f.section.end - f.section.start + 1, f.format);
    }
  }
  for (const FileLink& link : file_links) {
    if (!block.contains(link.section.start)) {
      continue;
    }
    int offset = link.section.start - block.position();
    setFormat(offset, link.section.end - link.section.start, link_format);
  }
}

BigTextAreaModel::BigTextAreaModel(QObject* parent)
    : QAbstractListModel(parent),
      cursor_follow_end(false),
      cursor_position(-1),
      current_line(0),
      selection_formatter(new SelectionFormatter(this, selection)),
      formatters({selection_formatter}) {
  connect(this, &BigTextAreaModel::cursorPositionChanged, this, [this] {
    if (cursor_position >= 0) {
      emit goToLine(GetCursorPositionLine());
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
  if (new_lines.isEmpty()) {
    return;
  }
  beginInsertRows(QModelIndex(), line_start_offsets.size(),
                  line_start_offsets.size() + new_lines.size() - 1);
  line_start_offsets.append(new_lines);
  this->text = text;
  endInsertRows();
  emit textChanged();
  if (cursor_follow_end) {
    emit goToLine(line_start_offsets.size() - 1);
  } else if (cursor_position >= 0) {
    emit goToLine(GetCursorPositionLine());
  }
}

QString BigTextAreaModel::GetText() const { return text; }

void BigTextAreaModel::SetFormatters(QList<TextFormatter*> formatters) {
  this->formatters = formatters;
  this->formatters.append(selection_formatter);
  emit formattersChanged();
}

QList<TextFormatter*> BigTextAreaModel::GetFormatters() const {
  return formatters;
}

int BigTextAreaModel::GetLineNumberMaxWidth() const {
  QFontMetrics m(font);
  return m.horizontalAdvance(QString::number(line_start_offsets.size()));
}

void BigTextAreaModel::selectInline(int line, int start, int end) {
  std::pair<int, int> redraw_range = selection.GetLineRange();
  selection.multiline_selection = false;
  selection.last_line = selection.first_line = line;
  selection.first_line_offset = start;
  selection.last_line_offset = end;
  emit rehighlightLines(redraw_range.first, redraw_range.second);
  emit rehighlightLines(line, line);
}

void BigTextAreaModel::selectLine(int line) {
  if (line < 0 || line >= line_start_offsets.size()) {
    return;
  }
  if (!selection.multiline_selection) {
    std::pair<int, int> redraw_range = selection.GetLineRange();
    selection.multiline_selection = true;
    selection.first_line = selection.last_line = line;
    selection.first_line_offset = selection.last_line_offset = -1;
    emit rehighlightLines(redraw_range.first, redraw_range.second);
    emit rehighlightLines(line, line);
  } else {
    std::pair<int, int> redraw_range = selection.GetLineRange();
    selection.last_line = line;
    emit rehighlightLines(redraw_range.first, redraw_range.second);
    redraw_range = selection.GetLineRange();
    emit rehighlightLines(redraw_range.first, redraw_range.second);
  }
}

void BigTextAreaModel::selectAll() {
  std::pair<int, int> redraw_range = selection.GetLineRange();
  selection = TextSelection();
  selection.first_line = 0;
  selection.last_line = line_start_offsets.size();
  emit rehighlightLines(redraw_range.first, redraw_range.second);
  redraw_range = selection.GetLineRange();
  emit rehighlightLines(redraw_range.first, redraw_range.second);
}

void BigTextAreaModel::selectSearchResult(int offset, int length) {
  std::pair<int, int> redraw_range = selection.GetLineRange();
  selection = TextSelection();
  if (length > 0) {
    int line = GetLineWithOffset(offset);
    selection.first_line = selection.last_line = line;
    selection.first_line_offset = offset - line_start_offsets[line];
    selection.last_line_offset = selection.first_line_offset + length;
    emit goToLine(line);
  }
  emit rehighlightLines(redraw_range.first, redraw_range.second);
  redraw_range = selection.GetLineRange();
  emit rehighlightLines(redraw_range.first, redraw_range.second);
}

bool BigTextAreaModel::resetSelection() {
  if (selection.first_line < 0) {
    return false;
  }
  std::pair<int, int> redraw_range = selection.GetLineRange();
  selection = TextSelection();
  emit rehighlightLines(redraw_range.first, redraw_range.second);
  return true;
}

void BigTextAreaModel::copySelection() {
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
  int end = line < line_start_offsets.size() - 1 ? line_start_offsets[line + 1]
                                                 : text.size();
  return std::max(end - start - 1, 0);
}

int BigTextAreaModel::GetLineWithOffset(int offset) const {
  for (int i = 0; i < line_start_offsets.size(); i++) {
    if (offset < line_start_offsets[i]) {
      return i - 1;
    }
  }
  return 0;
}

int BigTextAreaModel::GetCursorPositionLine() const {
  return GetLineWithOffset(cursor_position);
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
  format.setForeground(ViewSystem::BrushFromHex(kTheme.kColorBgBlack));
  format.setBackground(ViewSystem::BrushFromHex(kTheme.kColorHighlight));
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
    : QObject(parent), formatter(new SearchFormatter(this, results)) {}

bool TextSearchController::AreSearchResultsEmpty() const {
  return results.items.isEmpty();
}

void TextSearchController::search(const QString& term, const QString& text,
                                  bool select_result) {
  int prev_start = -1;
  if (selected_result < results.items.size()) {
    prev_start = results.items[selected_result].offset;
  }
  results = SearchResults();
  if (term.size() > 2) {
    int line = 0;
    int pos = 0;
    while (true) {
      int i = text.indexOf(term, pos, Qt::CaseSensitivity::CaseInsensitive);
      if (i < 0) {
        break;
      }
      line += text.sliced(pos, i - pos).count('\n');
      results.index[line].append(results.items.size());
      TextSegment result;
      result.offset = i;
      result.length = term.size();
      results.items.append(result);
      pos = result.offset + result.length;
    }
  }
  emit searchResultsChanged();
  if (selected_result >= results.items.size() ||
      results.items[selected_result].offset != prev_start) {
    selected_result = 0;
    for (int i = 0; i < results.items.size(); i++) {
      if (results.items[i].offset >= prev_start) {
        selected_result = i;
        break;
      }
    }
  }
  if (select_result) {
    if (!results.items.isEmpty()) {
      DisplaySelectedSearchResult();
    } else {
      emit selectResult(0, 0);
    }
  }
  UpdateSearchResultsCount();
}

void TextSearchController::replaceSearchResultWith(const QString& text,
                                                   bool replace_all) {
  if (results.items.isEmpty()) {
    return;
  }
  QList<int> offsets, lengths;
  if (replace_all) {
    for (auto it = results.items.rbegin(); it != results.items.rend(); it++) {
      offsets.append(it->offset);
      lengths.append(it->length);
    }
  } else {
    TextSegment result = results.items[selected_result];
    offsets.append(result.offset);
    lengths.append(result.length);
  }
  emit replaceResults(offsets, lengths, text);
}

void TextSearchController::goToResultWithStartAt(int text_position) {
  for (int i = 0; i < results.items.size(); i++) {
    if (results.items[i].offset == text_position) {
      selected_result = i;
      DisplaySelectedSearchResult();
      UpdateSearchResultsCount();
      break;
    }
  }
}

void TextSearchController::goToSearchResult(bool next) {
  if (results.items.isEmpty()) {
    return;
  }
  selected_result = next ? selected_result + 1 : selected_result - 1;
  if (selected_result >= results.items.size()) {
    selected_result = 0;
  } else if (selected_result < 0) {
    selected_result = results.items.size() - 1;
  }
  DisplaySelectedSearchResult();
  UpdateSearchResultsCount();
}

void TextSearchController::UpdateSearchResultsCount() {
  if (!results.items.isEmpty()) {
    search_results_count = QString::number(selected_result + 1) + " of " +
                           QString::number(results.items.size());
  } else {
    search_results_count = "No Results";
  }
  emit searchResultsCountChanged();
}

void TextSearchController::DisplaySelectedSearchResult() {
  TextSegment result = results.items[selected_result];
  emit selectResult(result.offset, result.length);
}

SearchFormatter::SearchFormatter(QObject* parent, const SearchResults& results)
    : TextFormatter(parent), results(results) {
  format.setBackground(ViewSystem::BrushFromHex("#6b420f"));
}

QList<TextFormat> SearchFormatter::Format(const QString&, LineInfo line) const {
  QList<TextFormat> fs;
  const QList<int>& sris = results.index[line.number];
  for (int sri : sris) {
    TextSegment sr = results.items[sri];
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
