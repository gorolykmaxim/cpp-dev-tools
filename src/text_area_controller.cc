#include "text_area_controller.h"

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

TextAreaModel::TextAreaModel(QObject* parent, TextAreaData& data)
    : QAbstractListModel(parent), text_data(data) {}

QHash<int, QByteArray> TextAreaModel::roleNames() const {
  return {{0, "text"}};
}

int TextAreaModel::rowCount(const QModelIndex&) const {
  return text_data.line_start_offsets.size();
}

QVariant TextAreaModel::data(const QModelIndex& index, int role) const {
  if (role != 0) {
    return QVariant();
  }
  int start = text_data.line_start_offsets[index.row()];
  int end = text_data.text.size();
  if (index.row() + 1 < text_data.line_start_offsets.size()) {
    end = text_data.line_start_offsets[index.row() + 1] - 1;
  }
  return text_data.text.sliced(start, end - start);
}

void TextAreaModel::DisplayText(const QString& text) {
  beginRemoveRows(QModelIndex(), 0, text_data.line_start_offsets.size() - 1);
  text_data.line_start_offsets.clear();
  text_data.text = "";
  endRemoveRows();
  int pos = 0;
  text_data.line_start_offsets.append(0);
  while (true) {
    int i = text.indexOf('\n', pos);
    if (i < 0) {
      break;
    }
    text_data.line_start_offsets.append(i + 1);
    pos = text_data.line_start_offsets.constLast();
  }
  beginInsertRows(QModelIndex(), 0, text_data.line_start_offsets.size() - 1);
  text_data.text = text;
  endInsertRows();
}

BigTextAreaController::BigTextAreaController(QObject* parent)
    : QObject(parent), text_model(new TextAreaModel(this, data)) {}

void BigTextAreaController::SetText(const QString& text) {
  text_model->DisplayText(text);
  emit textChanged();
}

QString BigTextAreaController::GetText() const { return data.text; }
