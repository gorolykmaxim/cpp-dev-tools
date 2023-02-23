#pragma once

#include <QString>
#include <QVariantList>

class Database {
 public:
  static void Initialize();
  static void ExecCmd(const QString& query, const QVariantList& args = {});
};
