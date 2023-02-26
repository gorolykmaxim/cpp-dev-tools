#pragma once

#include <QDateTime>
#include <QSqlQuery>
#include <QUuid>

class Project {
  Q_GADGET
  Q_PROPERTY(QString pathRelativeToHome READ GetPathRelativeToHome CONSTANT)
 public:
  static Project ReadFromSql(QSqlQuery& sql);
  QString GetPathRelativeToHome() const;
  QString GetFolderName() const;

  QUuid id;
  QString path;
  bool is_opened = false;
  QDateTime last_open_time;
};
