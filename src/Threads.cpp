#include "Threads.hpp"

void ScheduleIOTask(AppData& app,
                    const std::function<void()>& on_background,
                    const std::function<void()>& on_main) {
  (void) QtConcurrent::run(
      &app.io_thread_pool,
      [&app, on_background, on_main] () {
        on_background();
        QMetaObject::invokeMethod(&app.gui_app, [on_main] () {
          on_main();
        });
      });
}
