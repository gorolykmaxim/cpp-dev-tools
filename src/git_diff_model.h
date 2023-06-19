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
  Q_PROPERTY(
      bool isBeforeSelected MEMBER before_selected NOTIFY beforeSelectedChanged)
  Q_PROPERTY(SyntaxFormatter *formatter MEMBER formatter CONSTANT)
 public:
  explicit GitDiffModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QHash<int, QByteArray> roleNames() const;

 signals:
  void rawDiffChanged();
  void modelChanged();
  void beforeSelectedChanged();
  void fileChanged();

 private:
  void ParseDiff();

  QString raw_diff;
  int before_line_number_max_width;
  int after_line_number_max_width;
  QList<DiffLine> lines;
  bool before_selected;
  SyntaxFormatter *formatter;
  QString file;
};

#endif  // GITDIFFMODEL_H
