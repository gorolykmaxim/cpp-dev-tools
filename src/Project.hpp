#pragma once

#include <QDateTime>
#include <QSqlQuery>
#include <QUuid>

struct Project {
  static Project ReadFromSql(QSqlQuery& sql);
  QString GetPathRelativeToHome() const;
  QString GetFolderName() const;
  bool IsNull() const;
  bool operator==(const Project& other) const;
  bool operator!=(const Project& other) const;

  QUuid id;
  QString path;
  bool is_opened = false;
  QDateTime last_open_time;
};
