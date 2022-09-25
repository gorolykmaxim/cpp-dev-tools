#include "PerfTimer.hpp"

using namespace std::chrono;

PerfTimer::PerfTimer(const QString& name)
    : name(name),
      start(system_clock::now()) {}

PerfTimer::~PerfTimer() {
  system_clock::time_point now = system_clock::now();
  microseconds us = duration_cast<microseconds>(now - start);
  qDebug() << name << "took" << us.count() << "us";
}
