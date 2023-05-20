#ifndef IOTASK_H
#define IOTASK_H

#include <QtConcurrent>

#include "application.h"

class IoTask {
 public:
  template <typename T>
  static QFuture<T> Run(std::function<T()>&& callback) {
    return QtConcurrent::run(&Application::Get().io_thread_pool, callback);
  }

  template <typename T>
  static void Then(QFuture<T> future, QObject* requestor,
                   std::function<void(T)>&& callback) {
    auto watcher = new QFutureWatcher<T>(requestor);
    QObject::connect(watcher, &QFutureWatcher<T>::finished, requestor,
                     [callback = std::move(callback), watcher] {
                       callback(watcher->result());
                       watcher->deleteLater();
                     });
    watcher->setFuture(future);
  }

  static void Then(QFuture<void> future, QObject* requestor,
                   std::function<void()>&& callback);

  template <typename T>
  static void Run(QObject* requestor, std::function<T()>&& on_io_thread,
                  std::function<void(T)>&& on_ui_thread) {
    QFuture<T> future = Run(std::move(on_io_thread));
    Then(future, requestor, std::move(on_ui_thread));
  }

  static void Run(QObject* requestor, std::function<void()>&& on_io_thread,
                  std::function<void()>&& on_ui_thread);
};

#endif  // IOTASK_H
