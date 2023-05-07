#ifndef SQLITE_SYSTEM_H
#define SQLITE_SYSTEM_H

#include <QObject>
#include <QSqlQuery>
#include <QUuid>

struct SqliteFile {
  bool IsNull() const;
  bool operator==(const SqliteFile& another) const;
  bool operator!=(const SqliteFile& another) const;

  QUuid id;
  QString path;
  bool is_missing_on_disk = false;
};

struct SqliteQueryResult {
  QList<QString> columns;
  QList<QVariantList> rows;
  bool is_select = true;
  int rows_affected = 0;
  QString error;
};

class SqliteSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString selectedFileName READ GetSelectedFileName NOTIFY
                 selectedFileChanged)
  Q_PROPERTY(bool isFileSelected READ IsFileSelected NOTIFY selectedFileChanged)
 public:
  static SqliteFile ReadFromSql(QSqlQuery& sql);
  static void ExecuteQuery(
      QObject* requestor, const QString& query,
      const std::function<void(SqliteQueryResult)>& callback);
  static bool OpenIfExistsSync(QSqlDatabase& db, QString& error);
  void InitializeSelectedFile();
  void SetSelectedFile(const SqliteFile& file);
  SqliteFile GetSelectedFile() const;
  QString GetSelectedFileName() const;
  bool IsFileSelected() const;

  inline static const QString kConnectionName = "SQLite User Connection";

 signals:
  void selectedFileChanged();

 private:
  SqliteFile selected_file;
};

#endif  // SQLITE_SYSTEM_H
