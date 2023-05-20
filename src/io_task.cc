#include "io_task.h"

void IoTask::Then(QFuture<void> future, QObject *requestor,
                  std::function<void()> &&callback) {
  auto watcher = new QFutureWatcher<void>(requestor);
  QObject::connect(watcher, &QFutureWatcher<void>::finished, requestor,
                   [callback = std::move(callback), watcher] {
                     callback();
                     watcher->deleteLater();
                   });
  watcher->setFuture(future);
}

void IoTask::Run(QObject *requestor, std::function<void()> &&on_io_thread,
                 std::function<void()> &&on_ui_thread) {
  QFuture<void> future = Run(std::move(on_io_thread));
  Then(future, requestor, std::move(on_ui_thread));
}
