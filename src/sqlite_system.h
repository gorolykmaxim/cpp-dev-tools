#ifndef SQLITE_SYSTEM_H
#define SQLITE_SYSTEM_H

#include <QObject>

class SqliteSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString selectedFileName READ GetSelectedFileName NOTIFY
                 selectedFileChanged)
 public:
  void InitializeSelectedFile();
  void SetSelectedFile(const QString& file);
  QString GetSelectedFile() const;
  QString GetSelectedFileName() const;

  inline static const QString kConnectionName = "SQLite User Connection";

 signals:
  void selectedFileChanged();

 private:
  QString selected_file;
};

#endif  // SQLITE_SYSTEM_H
