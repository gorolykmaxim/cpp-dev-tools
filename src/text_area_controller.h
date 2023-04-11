#pragma once

#include <QObject>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>
#include <QtQmlIntegration>

struct TextSection {
  int start = 0;
  int end = 0;
};

struct TextSectionFormat {
  TextSection section;
  QTextCharFormat format;
};

class TextAreaFormatter : public QObject {
 public:
  explicit TextAreaFormatter(QObject* parent);
  virtual QList<TextSectionFormat> Format(const QString& text,
                                          const QTextBlock& block) = 0;
};

class TextAreaHighlighter : public QSyntaxHighlighter {
 public:
  TextAreaHighlighter();
  TextAreaFormatter* formatter;

 protected:
  void highlightBlock(const QString& text) override;
};

class TextAreaController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString searchResultsCount MEMBER search_results_count NOTIFY
                 searchResultsCountChanged)
  Q_PROPERTY(bool searchResultsEmpty READ AreSearchResultsEmpty NOTIFY
                 searchResultsCountChanged)
  Q_PROPERTY(QQuickTextDocument* document READ GetDocument WRITE SetDocument
                 NOTIFY documentChanged)
  Q_PROPERTY(TextAreaFormatter* formatter READ GetFormatter WRITE SetFormatter
                 NOTIFY formatterChanged)
 public:
  explicit TextAreaController(QObject* parent = nullptr);
  bool AreSearchResultsEmpty() const;
  QQuickTextDocument* GetDocument();
  void SetDocument(QQuickTextDocument* document);
  TextAreaFormatter* GetFormatter();
  void SetFormatter(TextAreaFormatter* formatter);
 public slots:
  void Search(const QString& term, const QString& text);
  void GoToResultWithStartAt(int text_position);
  void NextResult();
  void PreviousResult();
  void SaveCursorPosition(int position);
  void GoToPreviousCursorPosition();
  void GoToNextCursorPosition();
  void ResetCursorPositionHistory();

 signals:
  void selectText(int start, int end);
  void searchResultsCountChanged();
  void changeCursorPosition(int position);
  void documentChanged();
  void formatterChanged();

 private:
  void UpdateSearchResultsCount();
  void DisplaySelectedSearchResult();

  int selected_result = 0;
  QList<TextSection> search_results;
  QString search_results_count = "0 Results";
  QList<int> cursor_history;
  int cursor_history_index = -1;
  TextAreaHighlighter highlighter;
  QQuickTextDocument* document;
};
