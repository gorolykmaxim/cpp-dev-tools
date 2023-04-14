#include "text_area_controller.h"

#include "theme.h"

#define LOG() qDebug() << "[TextAreaController]"

static const int kCursorHistoryLimit = 100;

TextAreaController::TextAreaController(QObject* parent)
    : QObject(parent), highlighter(file_links) {
  cursor_history.reserve(kCursorHistoryLimit);
  UpdateSearchResultsCount();
}

void TextAreaController::search(const QString& term, const QString& text) {
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
      pos = result.end + 1;
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
  if (!search_results.isEmpty()) {
    DisplaySelectedSearchResult();
  } else {
    emit selectText(0, 0);
  }
  UpdateSearchResultsCount();
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

void TextAreaController::nextResult() {
  if (search_results.isEmpty()) {
    return;
  }
  if (++selected_result >= search_results.size()) {
    selected_result = 0;
  }
  DisplaySelectedSearchResult();
  UpdateSearchResultsCount();
}

void TextAreaController::previousResult() {
  if (search_results.isEmpty()) {
    return;
  }
  if (--selected_result < 0) {
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

void TextAreaController::goToPreviousCursorPosition() {
  if (cursor_history_index == 0) {
    return;
  }
  int pos = cursor_history[--cursor_history_index];
  emit changeCursorPosition(pos);
}

void TextAreaController::goToNextCursorPosition() {
  if (cursor_history_index == cursor_history.size() - 1) {
    return;
  }
  int pos = cursor_history[++cursor_history_index];
  emit changeCursorPosition(pos);
}

void TextAreaController::resetCursorPositionHistory() {
  cursor_history.clear();
  cursor_history_index = -1;
}

void TextAreaController::findFileLinks(const QString& text) {
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
  const FileLink* link = FindFileLinkAtPosition(pos);
  if (!link) {
    return;
  }
  QString link_str = link->file_path + ':' + QString::number(link->column);
  if (link->row >= 0) {
    link_str += ':' + QString::number(link->row);
  }
  LOG() << "Opening file link" << link_str;
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
  return FindFileLinkAtPosition(pos) != nullptr;
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

const FileLink* TextAreaController::FindFileLinkAtPosition(int position) const {
  for (const FileLink& link : file_links) {
    if (position >= link.section.start && position <= link.section.end) {
      return &link;
    }
  }
  return nullptr;
}

TextAreaFormatter::TextAreaFormatter(QObject* parent) : QObject(parent) {}

TextAreaHighlighter::TextAreaHighlighter(QList<FileLink>& file_links)
    : QSyntaxHighlighter((QObject*)nullptr), file_links(file_links) {
  Theme theme;
  QColor color = QColor::fromString(theme.kColorHighlight);
  link_format.setForeground(QBrush(color));
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
