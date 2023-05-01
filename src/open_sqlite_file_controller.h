#ifndef SELECTSQLITEFILECONTROLLER_H
#define SELECTSQLITEFILECONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

class OpenSqliteFileController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QUuid fileId MEMBER file_id NOTIFY fileIdChanged)
  Q_PROPERTY(QString title READ GetTitle WRITE SetTitle NOTIFY titleChanged)
 public:
  QString GetTitle() const;
  void SetTitle(const QString& value);
 public slots:
  void openDatabase(const QString& path);
 signals:
  void databaseOpened();
  void fileIdChanged();
  void titleChanged();

 private:
  QUuid file_id;
  QString title;
};

#endif  // SELECTSQLITEFILECONTROLLER_H
