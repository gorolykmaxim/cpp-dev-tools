#pragma once

#include "Lib.hpp"

class PerfTimer {
public:
  explicit PerfTimer(const QString& name);
  ~PerfTimer();
private:
  QString name;
  std::chrono::system_clock::time_point start;
};