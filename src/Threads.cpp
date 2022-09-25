#include "Threads.hpp"

Threads::Threads(QObject& ui_context) : ui_context(ui_context) {
  io_pool.setMaxThreadCount(1);
}

void Threads::ScheduleIO(const std::function<void()>& on_background,
                         const std::function<void()>& on_main) {
  (void) QtConcurrent::run(&io_pool, [=] () {
    on_background();
    QMetaObject::invokeMethod(&ui_context, [on_main] () {
      on_main();
    });
  });
}
