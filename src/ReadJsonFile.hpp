#pragma once

#include <QJsonDocument>
#include <QString>
#include "Application.hpp"

class ReadJsonFile: public Process {
public:
  ReadJsonFile(const QString& path);
  void Read(Application& app);

  QString path, error;
  QJsonDocument json;
};
