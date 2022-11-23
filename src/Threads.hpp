#pragma once

#include "AppData.hpp"

void ScheduleIOTask(AppData& app,
                    const std::function<void()>& on_background,
                    const std::function<void()>& on_main);

template<typename T>
void ScheduleIOTask(AppData& app,
                    const std::function<T()>& on_background,
                    const std::function<void(T)>& on_main) {
  (void) QtConcurrent::run(
      &app.io_thread_pool,
      [&app, on_background, on_main] () {
        T value = on_background();
        QMetaObject::invokeMethod(&app.gui_app, [on_main, value] () {
          on_main(value);
        });
      });
}
