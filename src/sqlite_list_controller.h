#ifndef DATABASELISTCONTROLLER_H
#define DATABASELISTCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "text_list_model.h"
#include "sqlite_system.h"

class SqliteFileListModel : public TextListModel {
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
  Q_PROPERTY(QUuid selectedDatabaseFileId READ GetSelectedDatabaseFileId NOTIFY
                 selectedDatabaseChanged)
  Q_PROPERTY(bool isDatabaseSelected READ IsDatabaseSelected NOTIFY
                 selectedDatabaseChanged)
 public:
  explicit SqliteListController(QObject* parent = nullptr);
  QUuid GetSelectedDatabaseFileId() const;
  bool IsDatabaseSelected() const;

 public slots:
  void displayDatabaseList();
  void useSelectedDatabase();
  void removeSelectedDatabase();
  void selectDatabase(int i);

 signals:
  void selectedDatabaseChanged();

 private:
  SqliteFileListModel* databases;
  SqliteFile selected;
};

#endif  // DATABASELISTCONTROLLER_H
