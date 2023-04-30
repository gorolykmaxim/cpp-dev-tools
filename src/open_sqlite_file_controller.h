#ifndef SELECTSQLITEFILECONTROLLER_H
#define SELECTSQLITEFILECONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

class OpenSqliteFileController : public QObject {
  Q_OBJECT
  QML_ELEMENT
 public:
  explicit OpenSqliteFileController(QObject* parent = nullptr);
 public slots:
  void openDatabase(const QString& path);
 signals:
  void databaseOpened();
};

#endif  // SELECTSQLITEFILECONTROLLER_H
