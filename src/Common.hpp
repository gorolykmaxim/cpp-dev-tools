#pragma once

#include <chrono>
#include <QSharedPointer>

template<typename T>
using QPtr = QSharedPointer<T>;

class PerfTimer {
public:
  explicit PerfTimer(const QString& name);
  ~PerfTimer();
private:
  QString name;
  std::chrono::system_clock::time_point start;
};
