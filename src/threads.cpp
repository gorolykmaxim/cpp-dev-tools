#include "threads.hpp"

Threads::Threads(QObject& ui_context) : ui_context(ui_context) {
  io_pool.setMaxThreadCount(1);
}
