#ifndef GITDIFFMODEL_H
#define GITDIFFMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

#include "syntax.h"
#include "text_area_controller.h"

struct DiffLine {
  int before_line_number = -1;
  int after_line_number = -1;
  bool is_delete = false;
  bool is_add = false;
  TextSegment before_line;
  TextSegment after_line;
  TextSegment header;
};

struct DiffSearchResult : public TextSegment {
  int line = 0;
  int side = 0;
};

class DiffSelectionFormatter : public TextFormatter {
 public:
  DiffSelectionFormatter(QObject *parent, const int &selected_side, int side,
                         const TextSelection &selection);
  QList<TextFormat> Format(const QString &text, LineInfo line) const;

 private:
  const int &selected_side;
  int side;
  SelectionFormatter formatter;
};

class DiffSearchFormatter : public TextFormatter {
 public:
  DiffSearchFormatter(QObject *parent, int side,
                      const QList<DiffSearchResult> &results,
                      const QHash<int, QList<int>> &index);
  QList<TextFormat> Format(const QString &text, LineInfo line) const;

 private:
  int side;
  const QList<DiffSearchResult> &results;
  const QHash<int, QList<int>> &index;
  QTextCharFormat format;
};

class GitDiffModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString file MEMBER file NOTIFY fileChanged)
  Q_PROPERTY(QString rawDiff MEMBER raw_diff NOTIFY rawDiffChanged)
  Q_PROPERTY(int maxLineNumberWidthBefore MEMBER before_line_number_max_width
                 NOTIFY modelChanged)
  Q_PROPERTY(int maxLineNumberWidthAfter MEMBER after_line_number_max_width
                 NOTIFY modelChanged)
  Q_PROPERTY(int selectedSide WRITE SetSelectedSide READ GetSelectedSide NOTIFY
                 selectedSideChanged)
  Q_PROPERTY(int chunkCount READ GetChunkCount NOTIFY modelChanged)
  Q_PROPERTY(int currentChunk MEMBER current_chunk NOTIFY currentChunkChanged)
  Q_PROPERTY(SyntaxFormatter *syntaxFormatter MEMBER syntax_formatter CONSTANT)
  Q_PROPERTY(DiffSelectionFormatter *beforeSelectionFormatter MEMBER
                 before_selection_formatter CONSTANT)
  Q_PROPERTY(DiffSelectionFormatter *afterSelectionFormatter MEMBER
                 after_selection_formatter CONSTANT)
  Q_PROPERTY(QString searchResultsCount READ GetSearchResultsCount NOTIFY
                 searchResultsCountChanged)
  Q_PROPERTY(bool searchResultsEmpty READ AreSearchResultsEmpty NOTIFY
                 searchResultsCountChanged)
  Q_PROPERTY(DiffSearchFormatter *beforeSearchFormatter MEMBER
                 before_search_formatter CONSTANT)
  Q_PROPERTY(DiffSearchFormatter *afterSearchFormatter MEMBER
                 after_search_formatter CONSTANT)
  Q_PROPERTY(bool isSideBySideView READ IsSideBySideView NOTIFY modelChanged)
 public:
  explicit GitDiffModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QHash<int, QByteArray> roleNames() const;
  void SetSelectedSide(int side);
  int GetSelectedSide() const;
  int GetChunkCount() const;
  QString GetSearchResultsCount() const;
  bool AreSearchResultsEmpty() const;
  bool IsSideBySideView() const;

 public slots:
  void selectInline(int line, int start, int end);
  void selectLine(int line);
  void selectAll();
  bool resetSelection();
  void copySelection(int current_line) const;
  void openFileInEditor(int current_line) const;
  void selectCurrentChunk(int current_line);
  void search(const QString &term);
  void goToSearchResult(bool next);
  void goToSearchResultInLineAndSide(int line, int side);
  QString getSelectedText() const;
  void toggleUnifiedView();

 signals:
  void rawDiffChanged();
  void modelChanged();
  void selectedSideChanged();
  void fileChanged();
  void rehighlightLines(int first, int last);
  void currentChunkChanged();
  void searchResultsCountChanged();
  void goToLine(int line);
  void isSideBySideViewChanged();

 private:
  void ParseDiff();
  QList<DiffLine> ParseIntoSideBySideDiff(int &max_before_line_number,
                                          int &max_after_line_number);
  QList<DiffLine> ParseIntoUnifiedDiff(int &max_before_line_number,
                                       int &max_after_line_number);
  void SelectCurrentSearchResult();
  QString GetSelectedTextInLine(const TextSelection &s, int i) const;

  QString raw_diff;
  int before_line_number_max_width;
  int after_line_number_max_width;
  QList<DiffLine> lines;
  int selected_side;
  SyntaxFormatter *syntax_formatter;
  DiffSelectionFormatter *before_selection_formatter;
  DiffSelectionFormatter *after_selection_formatter;
  QString file;
  TextSelection selection;
  QList<int> chunk_offsets;
  int current_chunk;
  QList<DiffSearchResult> search_results;
  QHash<int, QList<int>> search_result_index;
  int selected_result;
  DiffSearchFormatter *before_search_formatter;
  DiffSearchFormatter *after_search_formatter;
  bool side_by_side_view;
};

#endif  // GITDIFFMODEL_H
