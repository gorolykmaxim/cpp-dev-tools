#ifndef THREADS_H
#define THREADS_H

#include <QList>
#include <utility>

class Threads {
 public:
  static QList<std::pair<int, int>> SplitArrayAmongThreads(int size, int offset = 0);
};

#endif  // THREADS_H
