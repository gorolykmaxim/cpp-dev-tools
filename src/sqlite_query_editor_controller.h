#ifndef SQLITEQUERYEDITORCONTROLLER_H
#define SQLITEQUERYEDITORCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "sqlite_table_model.h"

class SqliteQueryEditorController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(SqliteTableModel* model MEMBER model CONSTANT)
  Q_PROPERTY(QString status MEMBER status NOTIFY statusChanged)
  Q_PROPERTY(QString statusColor MEMBER status_color NOTIFY statusChanged)
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
};

#endif  // SQLITEQUERYEDITORCONTROLLER_H
