#pragma once

#include <QObject>
#include <QtQmlIntegration>

struct SearchResult {
  int start = 0;
  int end = 0;
};

class TextAreaController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString searchResultsCount MEMBER search_results_count NOTIFY
                 searchResultsCountChanged)
  Q_PROPERTY(bool searchResultsEmpty READ AreSearchResultsEmpty NOTIFY
                 searchResultsCountChanged)
 public:
  explicit TextAreaController(QObject* parent = nullptr);
 public slots:
  void Search(const QString& term, const QString& text);
  void GoToResultWithStartAt(int text_position);
  void NextResult();
  void PreviousResult();
  bool AreSearchResultsEmpty() const;
 signals:
  void selectText(int start, int end);
  void searchResultsCountChanged();

 private:
  void UpdateSearchResultsCount();
  void DisplaySelectedSearchResult();

  int selected_result = 0;
  QList<SearchResult> search_results;
  QString search_results_count = "0 Results";
  QString search_term;
};
