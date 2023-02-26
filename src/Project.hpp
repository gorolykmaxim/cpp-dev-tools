#pragma once

#include <QDateTime>
#include <QSqlQuery>
#include <QUuid>

class Project {
 public:
  static Project ReadFromSql(QSqlQuery& sql);
  QString GetPathRelativeToHome() const;
  QString GetFolderName() const;

  QUuid id;
  QString path;
  bool is_opened = false;
  QDateTime last_open_time;
};
