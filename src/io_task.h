#ifndef IOTASK_H
#define IOTASK_H

#include <QtConcurrent>

#include "application.h"

template <typename T>
class Promise : public QFuture<T> {
 public:
  explicit Promise(const T& t) : QFuture<T>(QtFuture::makeReadyFuture(t)) {}
  explicit Promise(T&& t) : QFuture<T>(QtFuture::makeReadyFuture(t)) {}
  Promise(const QFuture<T>& another) : QFuture<T>(another) {}

  void Then(QObject* ctx, std::function<void(T)>&& cb) const {
    if (QFuture<T>::isFinished()) {
      cb(QFuture<T>::result());
      return;
    }
    auto w = new QFutureWatcher<T>(ctx);
    QObject::connect(w, &QFutureWatcher<T>::finished, ctx,
                     [cb = std::move(cb), w] {
                       cb(w->result());
                       w->deleteLater();
                     });
    w->setFuture(*this);
  }

  template <typename D>
  Promise<D> Then(QObject* ctx, std::function<QFuture<D>(T)>&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    if (QFuture<T>::isFinished()) {
      Promise<D> d = cb(QFuture<T>::result());
      d.Then(ctx, [qpd](D d) {
        qpd->addResult(std::move(d));
        qpd->finish();
      });
      return qpd->future();
    }
    auto w = new QFutureWatcher<T>(ctx);
    QObject::connect(w, &QFutureWatcher<T>::finished, ctx,
                     [cb = std::move(cb), qpd, w, ctx]() {
                       Promise<D> d = cb(w->result());
                       d.Then(ctx, [qpd](D d) {
                         qpd->addResult(std::move(d));
                         qpd->finish();
                       });
                       w->deleteLater();
                     });
    w->setFuture(*this);
    return qpd->future();
  }
};

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
