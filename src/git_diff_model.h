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
  Q_PROPERTY(SyntaxFormatter *syntaxFormatter MEMBER syntax_formatter CONSTANT)
  Q_PROPERTY(SelectionFormatter *selectionFormatter MEMBER selection_formatter
                 CONSTANT)
 public:
  explicit GitDiffModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QHash<int, QByteArray> roleNames() const;
  void SetSelectedSide(int side);
  int GetSelectedSide() const;

 public slots:
  void selectInline(int line, int start, int end);
  void selectLine(int line);
  void selectAll();
  bool resetSelection();
  void copySelection(int current_line);

 signals:
  void rawDiffChanged();
  void modelChanged();
  void selectedSideChanged();
  void fileChanged();
  void rehighlightLines(int first, int last);

 private:
  void ParseDiff();

  QString raw_diff;
  int before_line_number_max_width;
  int after_line_number_max_width;
  QList<DiffLine> lines;
  int selected_side;
  SyntaxFormatter *syntax_formatter;
  SelectionFormatter *selection_formatter;
  QString file;
  TextSelection selection;
};

#endif  // GITDIFFMODEL_H
