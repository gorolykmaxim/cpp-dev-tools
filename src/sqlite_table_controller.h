#ifndef SQLITETABLECONTROLLER_H
#define SQLITETABLECONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "text_list_model.h"
#include "sqlite_table_model.h"

struct Query {
  QString where;
  QString order_by;
  QString limit = "500";
  QString offset;
};

class SqliteTableController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(SimpleTextListModel* tables MEMBER tables CONSTANT)
  Q_PROPERTY(SqliteTableModel* table MEMBER table CONSTANT)
  Q_PROPERTY(QString status MEMBER status NOTIFY statusChanged)
  Q_PROPERTY(QString statusColor MEMBER status_color NOTIFY statusChanged)
  Q_PROPERTY(QString limit READ GetLimit NOTIFY displayTableView)
 public:
  explicit SqliteTableController(QObject* parent = nullptr);
  QString GetLimit() const;
 public slots:
  void displayTableList();
  void displayTable(int i);
  void load();
  void setLimit(const QString& value);
  void setOffset(const QString& value);
  void setOrderBy(const QString& value);
  void setWhere(const QString& value);
 signals:
  void displayTableView();
  void statusChanged();

 private:
  void SetStatus(const QString& status, const QString& color = "");

  SimpleTextListModel* tables;
  SqliteTableModel* table;
  QString table_name;
  QString status;
  QString status_color;
  Query query;
};

#endif  // SQLITETABLECONTROLLER_H
