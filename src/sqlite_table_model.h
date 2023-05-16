#ifndef SQLITETABLEMODEL_H
#define SQLITETABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QtQmlIntegration>

#include "placeholder.h"

class SqliteTableModel : public QAbstractTableModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      QString placeholderText READ GetPlaceholderText NOTIFY placeholderChanged)
  Q_PROPERTY(QString placeholderColor READ GetPlaceholderColor NOTIFY
                 placeholderChanged)
 public:
  explicit SqliteTableModel(QObject *parent);
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QHash<int, QByteArray> roleNames() const;
  void SetTable(const QList<QString> &new_columns,
                const QList<QVariantList> &new_rows);
  QString GetPlaceholderText() const;
  QString GetPlaceholderColor() const;
  void SetPlaceholder(const QString &text = "", const QString &color = "");

 public slots:
  bool moveCurrent(int key, int rows_visible);
  void setCurrent(int row, int column);
  void copyCurrentValue();
  void displayCurrentValueInDialog();

 signals:
  void currentChanged(QModelIndex index);
  void placeholderChanged();

 private:
  bool SetCurrent(QModelIndex next);

  QModelIndex current;
  QList<QString> columns;
  QList<QVariantList> rows;
  Placeholder placeholder;
};

#endif  // SQLITETABLEMODEL_H
