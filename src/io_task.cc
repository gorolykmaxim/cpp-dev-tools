#include "io_task.h"

void IoTask::Run(QObject *requestor, const std::function<void()> &on_io_thread,
                 const std::function<void()> &on_ui_thread) {
  Application &app = Application::Get();
  auto watcher = new QFutureWatcher<void>(requestor);
  QObject::connect(watcher, &QFutureWatcher<void>::finished, requestor,
                   [on_ui_thread, watcher] {
                     on_ui_thread();
                     watcher->deleteLater();
                   });
  QFuture<void> future = QtConcurrent::run(&app.io_thread_pool, on_io_thread);
  watcher->setFuture(future);
}
