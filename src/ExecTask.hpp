#pragma once

#include "AppData.hpp"

class ExecTask: public Process {
public:
  explicit ExecTask(const QString& task_name);
  void Schedule(AppData& app);
  void ExecNext(AppData& app);

  QString task_name;
  QUuid primary_exec_id;
};
