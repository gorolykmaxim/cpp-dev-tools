#pragma once

#include "AppData.hpp"
#include "JsonFileProcess.hpp"

class LoadTaskConfig: public Process {
public:
  LoadTaskConfig(const QString& path, bool create = false);
  void Load(AppData& app);
  void Read(AppData& app);

  bool success = false;
private:
  bool create;
  QString path;
  QPtr<JsonFileProcess> load_file;
};
