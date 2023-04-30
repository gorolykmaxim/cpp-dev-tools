#ifndef DATABASELISTCONTROLLER_H
#define DATABASELISTCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "qvariant_list_model.h"

struct SqliteFile {
  explicit SqliteFile(const QString& path = "",
                      bool is_missing_on_disk = false);

  QString path;
  bool is_missing_on_disk;
};

class SqliteFileListModel : public QVariantListModel {
 public:
  explicit SqliteFileListModel(QObject* parent);
  void SortAndLoad();

  QList<SqliteFile> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

class SqliteListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(SqliteFileListModel* databases MEMBER databases CONSTANT)
 public:
  explicit SqliteListController(QObject* parent = nullptr);

 public slots:
  void selectDatabase(int i);
  void removeDatabase(int i);

 private:
  SqliteFileListModel* databases;
};

#endif  // DATABASELISTCONTROLLER_H
