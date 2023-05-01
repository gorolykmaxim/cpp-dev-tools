#ifndef SELECTSQLITEFILECONTROLLER_H
#define SELECTSQLITEFILECONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

class OpenSqliteFileController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QUuid fileId MEMBER file_id NOTIFY fileIdChanged)
 public slots:
  void openDatabase(const QString& path);
 signals:
  void databaseOpened();
  void fileIdChanged();

 private:
  QUuid file_id;
};

#endif  // SELECTSQLITEFILECONTROLLER_H
