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
  Q_PROPERTY(QString status MEMBER status NOTIFY statusChanged)
  Q_PROPERTY(QString statusColor MEMBER status_color NOTIFY statusChanged)
  Q_PROPERTY(SyntaxFormatter* formatter MEMBER formatter CONSTANT)
 public:
  explicit SqliteQueryEditorController(QObject* parent = nullptr);

 public slots:
  void executeQuery(const QString& text, int cursor);

 signals:
  void statusChanged();

 private:
  void SetStatus(const QString& status, const QString& color = "");

  SqliteTableModel* model;
  QString status;
  QString status_color;
  SyntaxFormatter* formatter;
};

#endif  // SQLITEQUERYEDITORCONTROLLER_H
