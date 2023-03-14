#include "TextAreaController.hpp"

TextAreaController::TextAreaController(QObject* parent) : QObject(parent) {
  UpdateSearchResultsCount();
}

void TextAreaController::Search(const QString& term, const QString& text) {
  search_results.clear();
  if (term.size() > 2) {
    int pos = 0;
    while (true) {
      int i = text.indexOf(term, pos, Qt::CaseSensitivity::CaseInsensitive);
      if (i < 0) {
        break;
      }
      SearchResult result;
      result.start = i;
      result.end = i + term.size();
      search_results.append(result);
      pos = result.end + 1;
    }
  }
  // If the search_term didn't change and text just got bigger - we don't want
  // to reset selected_result: TextArea might be displaying an active log and
  // when new lines are added to it - we want to preserve selected_result.
  if (search_term != term || selected_result >= search_results.size()) {
    selected_result = 0;
  }
  search_term = term;
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

bool TextAreaController::AreSearchResultsEmpty() const {
  return search_results.isEmpty();
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
  const SearchResult& result = search_results[selected_result];
  emit selectText(result.start, result.end);
}
