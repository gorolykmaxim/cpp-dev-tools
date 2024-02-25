#include "threads.h"
#include <QThreadPool>

QList<std::pair<int, int>> Threads::SplitArrayAmongThreads(int size, int offset) {
  int threads = QThreadPool::globalInstance()->maxThreadCount();
  int count_per_thread = size / threads;
  QList<std::pair<int, int>> ranges;
  int start = offset;
  int end = offset;
  for (int i = 0; i < threads - 1; i++) {
    start = offset + i * count_per_thread;
    end = start + count_per_thread;
    ranges.append(std::make_pair(start, end));
  }
  start = end;
  end = size;
  ranges.append(std::make_pair(start, end));
  return ranges;
}
