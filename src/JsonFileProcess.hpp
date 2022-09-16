#pragma once

#include <QString>
#include <QJsonDocument>
#include "ProcessRuntime.hpp"
#include "Application.hpp"

enum class JsonOperation {
  kRead, kWrite
};

class JsonFileProcess: public Process {
public:
  JsonFileProcess(JsonOperation operation, const QString& path,
                  QJsonDocument json = QJsonDocument());
  void Run(Application& app);

  JsonOperation operation;
  QString path, error;
  QJsonDocument json;
};
