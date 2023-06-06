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
  QList<FileLink> file_links;
  bool detect_file_links;
};

struct TextAreaData {
  int GetLineLength(int line) const;

  QString text;
  QList<int> line_start_offsets;
};

struct TextPoint {
  TextPoint();
  TextPoint(int line, int offset);
  bool IsNull() const;
  bool operator==(const TextPoint& another) const;
  bool operator!=(const TextPoint& another) const;

  int line;
  int offset;
};

class TextAreaModel : public QAbstractListModel {
 public:
  TextAreaModel(QObject* parent, TextAreaData& data);
  QHash<int, QByteArray> roleNames() const;
  int rowCount(const QModelIndex& parent) const;
  QVariant data(const QModelIndex& index, int role) const;
  void DisplayText(const QString& text);

 private:
  TextAreaData& text_data;
};

class BigTextAreaController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString text WRITE SetText READ GetText NOTIFY textChanged)
  Q_PROPERTY(TextAreaModel* textModel MEMBER text_model CONSTANT)
  Q_PROPERTY(bool cursorFollowEnd MEMBER cursor_follow_end NOTIFY
                 cursorFollowEndChanged)
  Q_PROPERTY(int currentLine MEMBER current_line NOTIFY currentLineChanged)
 public:
  explicit BigTextAreaController(QObject* parent = nullptr);
  void SetText(const QString& text);
  QString GetText() const;

 public slots:
  void selectInline(int line, int start, int end);
  void copySelection();

 signals:
  void textChanged();
  void cursorFollowEndChanged();
  void goToLine(int line);
  void currentLineChanged();

 private:
  TextAreaData data;
  bool cursor_follow_end;
  TextAreaModel* text_model;
  TextPoint selection_start, selection_end;
  int current_line;
};
