#pragma once

#include "AppData.hpp"
#include "ExecOSCmd.hpp"

class ExecTask : public Process {
 public:
  explicit ExecTask(const QString& cmd);
  void Start(AppData& app);
  void Finish(AppData& app);

  QString cmd;
  QUuid exec_id;
  QPtr<ExecOSCmd> task_proc;
};
