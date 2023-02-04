#include "PerfTimer.hpp"

#define LOG() qDebug() << "[PerfTimer]"

using namespace std::chrono;

PerfTimer::PerfTimer(const QString& name)
    : name(name), start(system_clock::now()) {}

void PerfTimer::Measure() const {
  system_clock::time_point now = system_clock::now();
  microseconds us = duration_cast<microseconds>(now - start);
  LOG() << name << "took" << us.count() << "us";
}

PerfTimer::~PerfTimer() { Measure(); }
