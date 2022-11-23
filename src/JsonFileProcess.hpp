#pragma once

#include "Lib.hpp"
#include "AppData.hpp"

enum class JsonOperation {
  kRead, kWrite
};

class JsonFileProcess: public Process {
public:
  JsonFileProcess(JsonOperation operation, const QString& path,
                  QJsonDocument json = QJsonDocument());
  void Run(AppData& app);

  JsonOperation operation;
  QString path, error;
  QJsonDocument json;
};
