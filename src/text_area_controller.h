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

struct OldFileLink {
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
  TextAreaHighlighter(QList<OldFileLink>& file_links);
  TextAreaFormatter* formatter = nullptr;
  QList<OldFileLink>& file_links;
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
  void replaceSearchResultWith(const QString& text, bool replace_all);
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
  void replaceText(int start, int end, const QString& text);
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
  QList<OldFileLink> file_links;
  bool detect_file_links;
};

struct TextSegment {
  int offset = 0;
  int length = 0;
};

struct TextFormat : public TextSegment {
  QTextCharFormat format;
};

struct LineInfo {
  int offset = 0;
  int number = 0;
};

struct FileLink : public TextSegment {
  int line = 0;
  QString file_path;
  int column = -1;
  int row = -1;
};

struct TextSelection {
  TextSelection();
  std::pair<int, int> GetLineRange() const;
  TextSelection Normalize() const;

  int first_line;
  int first_line_offset;
  int last_line;
  int last_line_offset;
  bool multiline_selection;
};

class TextFormatter : public QObject {
 public:
  explicit TextFormatter(QObject* parent);
  virtual QList<TextFormat> Format(const QString& text,
                                   LineInfo line) const = 0;
};

class DummyFormatter : public TextFormatter {
  Q_OBJECT
  QML_ELEMENT
 public:
  explicit DummyFormatter(QObject* parent = nullptr);
  QList<TextFormat> Format(const QString& text, LineInfo line) const;
};

class LineHighlighter : public QSyntaxHighlighter {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QList<TextFormatter*> formatters MEMBER formatters NOTIFY
                 formattersChanged)
  Q_PROPERTY(QQuickTextDocument* document READ GetDocument WRITE SetDocument
                 NOTIFY documentChanged)
  Q_PROPERTY(int lineNumber MEMBER line_number NOTIFY lineChanged)
  Q_PROPERTY(int lineOffset MEMBER line_offset NOTIFY lineChanged)
 public:
  explicit LineHighlighter(QObject* parent = nullptr);
  QQuickTextDocument* GetDocument() const;
  void SetDocument(QQuickTextDocument* document);

 protected:
  void highlightBlock(const QString& text);

 signals:
  void formattersChanged();
  void documentChanged();
  void lineChanged();

 private:
  QList<TextFormatter*> formatters;
  QQuickTextDocument* document;
  int line_number;
  int line_offset;
};

class SelectionFormatter : public TextFormatter {
 public:
  SelectionFormatter(QObject* parent, const TextSelection& selection);
  QList<TextFormat> Format(const QString& text, LineInfo line) const;

 private:
  const TextSelection& selection;
  QTextCharFormat format;
};

class SearchFormatter : public TextFormatter {
 public:
  SearchFormatter(QObject* parent, const QList<TextSegment>& results,
                  const QHash<int, QList<int>>& index);
  QList<TextFormat> Format(const QString& text, LineInfo line) const;

 private:
  const QList<TextSegment>& results;
  const QHash<int, QList<int>>& index;
  QTextCharFormat format;
};

class TextSearchController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString searchResultsCount MEMBER search_results_count NOTIFY
                 searchResultsCountChanged)
  Q_PROPERTY(bool searchResultsEmpty READ AreSearchResultsEmpty NOTIFY
                 searchResultsCountChanged)
  Q_PROPERTY(SearchFormatter* formatter MEMBER formatter CONSTANT)
 public:
  explicit TextSearchController(QObject* parent = nullptr);
  bool AreSearchResultsEmpty() const;

 public slots:
  void search(const QString& term, const QString& text, bool select_result,
              bool notify_results_changed);
  void replaceSearchResultWith(const QString& text, bool replace_all);
  void goToResultWithStartAt(int text_position);
  void goToSearchResult(bool next);

 signals:
  void selectResult(int offset, int length);
  void replaceResults(const QList<int>& offsets, const QList<int>& lengths,
                      const QString& text);
  void searchResultsCountChanged();
  void searchResultsChanged();

 private:
  void UpdateSearchResultsCount();
  void DisplaySelectedSearchResult();

  int selected_result = 0;
  QList<TextSegment> results;
  QHash<int, QList<int>> index;
  QString search_results_count = "0 Results";
  SearchFormatter* formatter;
};

class FileLinkFormatter : public TextFormatter {
 public:
  FileLinkFormatter(QObject* parent, const QList<FileLink>& links,
                    const QHash<int, QList<int>>& index,
                    const int& current_line, const int& current_line_link);
  QList<TextFormat> Format(const QString& text, LineInfo line) const;

 private:
  const QList<FileLink>& links;
  const QHash<int, QList<int>>& index;
  const int& current_line;
  const int& current_line_link;
  QTextCharFormat format;
};

class FileLinkLookupController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(FileLinkFormatter* formatter MEMBER formatter CONSTANT)
 public:
  explicit FileLinkLookupController(QObject* parent = nullptr);

 public slots:
  void findFileLinks(const QString& text);
  void setCurrentLine(int line);
  void openCurrentFileLink();
  void goToLink(bool next);

 signals:
  void rehighlightLine(int line);
  void linkInLineSelected(int line);

 private:
  void FindFileLinks(const QRegularExpression& regex, const QString& text);

  int current_line;
  int current_line_link;
  FileLinkFormatter* formatter;
  QList<FileLink> links;
  QHash<int, QList<int>> index;
};

class BigTextAreaModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString text WRITE SetText READ GetText NOTIFY textChanged)
  Q_PROPERTY(bool cursorFollowEnd MEMBER cursor_follow_end NOTIFY
                 cursorFollowEndChanged)
  Q_PROPERTY(
      int cursorPosition MEMBER cursor_position NOTIFY cursorPositionChanged)
  Q_PROPERTY(int currentLine MEMBER current_line NOTIFY currentLineChanged)
  Q_PROPERTY(QList<TextFormatter*> formatters WRITE SetFormatters READ
                 GetFormatters NOTIFY formattersChanged)
  Q_PROPERTY(QFont font MEMBER font NOTIFY fontChanged)
  Q_PROPERTY(
      int lineNumberMaxWidth READ GetLineNumberMaxWidth NOTIFY fontChanged)
 public:
  explicit BigTextAreaModel(QObject* parent = nullptr);
  QHash<int, QByteArray> roleNames() const;
  int rowCount(const QModelIndex& parent) const;
  QVariant data(const QModelIndex& index, int role) const;
  void SetText(const QString& text);
  QString GetText() const;
  void SetFormatters(QList<TextFormatter*> formatters);
  QList<TextFormatter*> GetFormatters() const;
  int GetLineNumberMaxWidth() const;

 public slots:
  void selectInline(int line, int start, int end);
  void selectLine(int line);
  void selectAll();
  void selectSearchResult(int offset, int length);
  bool resetSelection();
  void copySelection();
  int getSelectionOffset();
  QString getSelectedText();
  void rehighlight();
  void rehighlightLine(int line);

 signals:
  void textChanged();
  void cursorFollowEndChanged();
  void goToLine(int line);
  void currentLineChanged();
  void rehighlightLines(int first, int last);
  void formattersChanged();
  void cursorPositionChanged();
  void fontChanged();

 private:
  int GetLineLength(int line) const;
  int GetLineWithOffset(int offset) const;
  int GetCursorPositionLine() const;
  std::pair<int, int> GetSelectionRange(const TextSelection& s) const;

  bool cursor_follow_end;
  int cursor_position;
  int current_line;
  QString text;
  QList<int> line_start_offsets;
  TextSelection selection;
  SelectionFormatter* selection_formatter;
  QList<TextFormatter*> formatters;
  QFont font;
};

class SmallTextAreaHighlighter : public QSyntaxHighlighter {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QList<TextFormatter*> formatters MEMBER formatters NOTIFY
                 formattersChanged)
  Q_PROPERTY(QQuickTextDocument* document READ GetDocument WRITE SetDocument
                 NOTIFY documentChanged)
 public:
  explicit SmallTextAreaHighlighter(QObject* parent = nullptr);
  QQuickTextDocument* GetDocument() const;
  void SetDocument(QQuickTextDocument* document);

 protected:
  void highlightBlock(const QString& text);

 signals:
  void formattersChanged();
  void documentChanged();

 private:
  QList<TextFormatter*> formatters;
  QQuickTextDocument* document;
};
