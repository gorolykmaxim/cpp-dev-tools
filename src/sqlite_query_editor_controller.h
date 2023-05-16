#ifndef SQLITEQUERYEDITORCONTROLLER_H
#define SQLITEQUERYEDITORCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "sqlite_table_model.h"
#include "syntax.h"

class SqliteQueryEditorController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(SqliteTableModel* model MEMBER model CONSTANT)
  Q_PROPERTY(QString query WRITE SaveQuery READ GetQuery NOTIFY queryChanged)
  Q_PROPERTY(SyntaxFormatter* formatter MEMBER formatter CONSTANT)
 public:
  explicit SqliteQueryEditorController(QObject* parent = nullptr);
  void SaveQuery(const QString& query);
  QString GetQuery() const;

 public slots:
  void executeQuery(const QString& text, int cursor);

 signals:
  void queryChanged();

 private:
  SqliteTableModel* model;
  QString query;
  SyntaxFormatter* formatter;
};

#endif  // SQLITEQUERYEDITORCONTROLLER_H
