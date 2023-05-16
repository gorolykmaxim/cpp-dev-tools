#ifndef SQLITETABLECONTROLLER_H
#define SQLITETABLECONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "sqlite_table_model.h"
#include "text_list_model.h"

struct Query {
  QString where;
  QString order_by;
  QString limit = "500";
  QString offset;
};

class TableListModel : public TextListModel {
 public:
  explicit TableListModel(QObject* parent);
  QString GetSelected() const;

  QStringList list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

class SqliteTableController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(TableListModel* tables MEMBER tables CONSTANT)
  Q_PROPERTY(SqliteTableModel* table MEMBER table CONSTANT)
  Q_PROPERTY(QString limit READ GetLimit NOTIFY displayTableView)
 public:
  explicit SqliteTableController(QObject* parent = nullptr);
  QString GetLimit() const;
 public slots:
  void displayTableList();
  void displaySelectedTable();
  void load();
  void setLimit(const QString& value);
  void setOffset(const QString& value);
  void setOrderBy(const QString& value);
  void setWhere(const QString& value);
 signals:
  void displayTableView();

 private:
  TableListModel* tables;
  SqliteTableModel* table;
  Query query;
};

#endif  // SQLITETABLECONTROLLER_H
