#pragma once

#include <QJsonDocument>
#include <QString>
#include "Application.hpp"

class WriteJsonFile: public Process {
public:
  WriteJsonFile(const QString& path, const QJsonDocument& json);
  void Write(Application& app);
private:
  QString path, error;
  QJsonDocument json;
};
