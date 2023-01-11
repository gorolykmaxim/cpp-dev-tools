#pragma once

#include "AppData.hpp"

class ExecOSCmd: public Process {
public:
  explicit ExecOSCmd(const QString& cmd, QPtr<QProcess> proc = nullptr);
  void Run(AppData& app);

  QString cmd;
  QPtr<QProcess> proc;
  std::function<void(const QString&)> on_output;
};
