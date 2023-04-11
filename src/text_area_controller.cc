#include "text_area_controller.h"

#include "theme.h"

static const int kCursorHistoryLimit = 100;

TextAreaController::TextAreaController(QObject* parent)
    : QObject(parent), highlighter(file_links) {
  cursor_history.reserve(kCursorHistoryLimit);
  UpdateSearchResultsCount();
}

void TextAreaController::Search(const QString& term, const QString& text) {
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

void TextAreaController::GoToResultWithStartAt(int text_position) {
  for (int i = 0; i < search_results.size(); i++) {
    if (search_results[i].start == text_position) {
      selected_result = i;
      DisplaySelectedSearchResult();
      UpdateSearchResultsCount();
      break;
    }
  }
}

void TextAreaController::NextResult() {
  if (search_results.isEmpty()) {
    return;
  }
  if (++selected_result >= search_results.size()) {
    selected_result = 0;
  }
  DisplaySelectedSearchResult();
  UpdateSearchResultsCount();
}

void TextAreaController::PreviousResult() {
  if (search_results.isEmpty()) {
    return;
  }
  if (--selected_result < 0) {
    selected_result = search_results.size() - 1;
  }
  DisplaySelectedSearchResult();
  UpdateSearchResultsCount();
}

void TextAreaController::SaveCursorPosition(int position) {
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
}

void TextAreaController::GoToPreviousCursorPosition() {
  if (cursor_history_index == 0) {
    return;
  }
  int pos = cursor_history[--cursor_history_index];
  emit changeCursorPosition(pos);
}

void TextAreaController::GoToNextCursorPosition() {
  if (cursor_history_index == cursor_history.size() - 1) {
    return;
  }
  int pos = cursor_history[++cursor_history_index];
  emit changeCursorPosition(pos);
}

void TextAreaController::ResetCursorPositionHistory() {
  cursor_history.clear();
  cursor_history_index = -1;
}

void TextAreaController::FindFileLinks(const QString& text) {
  static const QRegularExpression regex(
      "(\\/[^:]+):([0-9]+):?([0-9]+)?|"
      "([A-Z]\\:\\\\[^:]+)\\(([0-9]+),?([0-9]+)?\\)|"
      "([A-Z]\\:\\\\[^:]+):([0-9]+):([0-9]+)?");
  file_links.clear();
  for (auto it = regex.globalMatch(text); it.hasNext();) {
    QRegularExpressionMatch m = it.next();
    TextSection link;
    link.start = m.capturedStart();
    link.end = m.capturedEnd();
    file_links.append(link);
  }
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

TextAreaFormatter::TextAreaFormatter(QObject* parent) : QObject(parent) {}

TextAreaHighlighter::TextAreaHighlighter(QList<TextSection>& file_links)
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
  for (TextSection link : file_links) {
    if (!block.contains(link.start)) {
      continue;
    }
    int offset = link.start - block.position();
    setFormat(offset, link.end - link.start, link_format);
  }
}
