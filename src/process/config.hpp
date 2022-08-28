#pragma once

#include <QJsonDocument>
#include <QFile>
#include "application.hpp"

class ReadJsonFile: public Process {
public:
  ReadJsonFile(const QString& path);
  void Read(Application& app);

  QString error;
  QJsonDocument json;
private:
  QFile file;
};

class Initialize: public Process {
public:
  Initialize();
  void ReadConfigs(Application& app);
  void DisplayConfigs(Application& app);
private:
  QPtr<ReadJsonFile> read_user_config, read_task_config;
};
