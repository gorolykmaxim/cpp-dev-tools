#pragma once

#include <QJsonDocument>
#include <QString>
#include "application.hpp"

class ReadJsonFile: public Process {
public:
  ReadJsonFile(const QString& path);
  void Read(Application& app);

  QString path, error;
  QJsonDocument json;
};

class WriteJsonFile: public Process {
public:
  WriteJsonFile(const QString& path, const QJsonDocument& json);
  void Write(Application& app);
private:
  QString path, error;
  QJsonDocument json;
};

class Initialize: public Process {
public:
  Initialize();
  void ReadConfig(Application& app);
  void LoadConfig(Application& app);
private:
  QPtr<ReadJsonFile> read_config;
};
