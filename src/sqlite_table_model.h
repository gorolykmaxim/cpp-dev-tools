#ifndef SQLITETABLEMODEL_H
#define SQLITETABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QtQmlIntegration>

class SqliteTableModel : public QAbstractTableModel {
  Q_OBJECT
  QML_ELEMENT
 public:
  explicit SqliteTableModel(QObject *parent);
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QHash<int, QByteArray> roleNames() const;
  void SetTable(const QList<QString> &new_columns,
                const QList<QVariantList> &new_rows);

 public slots:
  bool moveCurrent(int key, int rows_visible);
  void setCurrent(int row, int column);
  void copyCurrentValue();
  void displayCurrentValueInDialog();

 signals:
  void currentChanged(QModelIndex index);

 private:
  bool SetCurrent(QModelIndex next);

  QModelIndex current;
  QList<QString> columns;
  QList<QVariantList> rows;
};

#endif  // SQLITETABLEMODEL_H
