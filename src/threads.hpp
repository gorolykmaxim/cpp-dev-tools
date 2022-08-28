#pragma once

#include <QThreadPool>
#include <QtConcurrent>
#include <QMetaObject>
#include <functional>

class Threads {
public:
  Threads(QObject& ui_context);
  void ScheduleIO(const std::function<void()>& on_background,
                  const std::function<void()>& on_main);

  template<typename T>
  void ScheduleIO(const std::function<T()>& on_background,
                  const std::function<void(T)>& on_main) {
    (void) QtConcurrent::run(&io_pool, [=] () {
      T value = on_background();
      QMetaObject::invokeMethod(&ui_context, [=] () {
        on_main(value);
      });
    });
  }

private:
  QObject& ui_context;
  QThreadPool io_pool;
};
