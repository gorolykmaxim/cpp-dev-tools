#ifndef IOTASK_H
#define IOTASK_H

#include <QtConcurrent>

#include "application.h"

class IoTask {
 public:
  template <typename T>
  static void Run(QObject* requestor, const std::function<T()>& on_io_thread,
                  const std::function<void(T)>& on_ui_thread) {
    Application& app = Application::Get();
    auto watcher = new QFutureWatcher<T>(requestor);
    QObject::connect(watcher, &QFutureWatcher<T>::finished, requestor,
                     [on_ui_thread, watcher] {
                       on_ui_thread(watcher->result());
                       watcher->deleteLater();
                     });
    QFuture<T> future = QtConcurrent::run(&app.io_thread_pool, on_io_thread);
    watcher->setFuture(future);
  }

  static void Run(QObject* requestor, const std::function<void()>& on_io_thread,
                  const std::function<void()>& on_ui_thread);
};

#endif  // IOTASK_H
