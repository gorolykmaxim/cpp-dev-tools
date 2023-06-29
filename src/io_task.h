#ifndef IOTASK_H
#define IOTASK_H

#include <QtConcurrent>

#include "application.h"
#include "promise.h"

class IoTask {
 public:
  static Promise<void> Run(std::function<void()>&& cb) {
    return QtConcurrent::run(&Application::Get().io_thread_pool, std::move(cb));
  }

  template <typename T>
  static QFuture<T> Run(std::function<T()>&& cb) {
    return QtConcurrent::run(&Application::Get().io_thread_pool, std::move(cb));
  }

  template <typename T>
  static QFuture<T> Run(std::function<void(QPromise<T>&)>&& cb) {
    return QtConcurrent::run(&Application::Get().io_thread_pool, std::move(cb));
  }

  template <typename T>
  static void Run(QObject* requestor, std::function<T()>&& on_io_thread,
                  std::function<void(T)>&& on_ui_thread) {
    Promise<T> p = Run(std::move(on_io_thread));
    p.Then(requestor, std::move(on_ui_thread));
  }

  static void Run(QObject* requestor, std::function<void()>&& on_io_thread,
                  std::function<void()>&& on_ui_thread) {
    Promise<void> p = Run(std::move(on_io_thread));
    p.Then(requestor, std::move(on_ui_thread));
  }
};

#endif  // IOTASK_H
