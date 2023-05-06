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

struct FileLink {
  TextSection section;
  QString file_path;
  int column = -1;
  int row = -1;
};

class TextAreaFormatter : public QObject {
 public:
  explicit TextAreaFormatter(QObject* parent);
  virtual QList<TextSectionFormat> Format(const QString& text,
                                          const QTextBlock& block) = 0;
};

class TextAreaHighlighter : public QSyntaxHighlighter {
 public:
  TextAreaHighlighter(QList<FileLink>& file_links);
  TextAreaFormatter* formatter = nullptr;
  QList<FileLink>& file_links;
  QTextCharFormat link_format;

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
  Q_PROPERTY(
      bool isCursorOnLink READ IsCursorOnLink NOTIFY cursorPositionChanged)
  Q_PROPERTY(bool detectFileLinks MEMBER detect_file_links NOTIFY
                 detectFileLinksChanged)
 public:
  explicit TextAreaController(QObject* parent = nullptr);
  bool AreSearchResultsEmpty() const;
  QQuickTextDocument* GetDocument();
  void SetDocument(QQuickTextDocument* document);
  TextAreaFormatter* GetFormatter();
  void SetFormatter(TextAreaFormatter* formatter);
  bool IsCursorOnLink() const;
 public slots:
  void search(const QString& term, const QString& text, bool select_result);
  void goToResultWithStartAt(int text_position);
  void goToSearchResult(bool next);
  void saveCursorPosition(int position);
  void goToCursorPosition(bool next);
  void resetCursorPositionHistory();
  void findFileLinks(const QString& text);
  void openFileLinkAtCursor();
  void goToFileLink(bool next);
  void rehighlightBlockByLineNumber(int i);
  void goToPage(const QString& text, bool up);

 signals:
  void selectText(int start, int end);
  void searchResultsCountChanged();
  void changeCursorPosition(int position);
  void documentChanged();
  void formatterChanged();
  void cursorPositionChanged();
  void detectFileLinksChanged();

 private:
  void UpdateSearchResultsCount();
  void DisplaySelectedSearchResult();
  void FindFileLinks(const QRegularExpression& regex, const QString& text);
  int IndexOfFileLinkAtPosition(int position) const;

  int selected_result = 0;
  QList<TextSection> search_results;
  QString search_results_count = "0 Results";
  QList<int> cursor_history;
  int cursor_history_index = -1;
  TextAreaHighlighter highlighter;
  QQuickTextDocument* document;
  QList<FileLink> file_links;
  bool detect_file_links;
};
