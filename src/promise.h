#ifndef PROMISE_H
#define PROMISE_H

#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>
#include <QSharedPointer>

template <typename T>
class Promise : public QFuture<T> {
 public:
  template <typename... Promises>
  static Promise<QList<T>> All(QObject* ctx, Promises&&... args) {
    QList<Promise<T>> ps = {args...};
    auto i = QSharedPointer<int>::create(ps.size());
    auto result = QSharedPointer<QList<T>>::create();
    auto qp = QSharedPointer<QPromise<QList<T>>>::create();
    for (const Promise<T>& p : ps) {
      p.Then(ctx, [i, qp, result](T t) {
        (*i)--;
        result->append(std::move(t));
        if (*i == 0) {
          qp->addResult(*result);
          qp->finish();
        }
      });
    }
    return qp->future();
  }

  Promise() : QFuture<T>() {}
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

template <>
class Promise<void> : public QFuture<void> {
 public:
  template <typename... Promises>
  static Promise<void> All(QObject* ctx, Promises&&... args) {
    QList<Promise<void>> ps = {args...};
    auto i = QSharedPointer<int>::create(ps.size());
    auto qp = QSharedPointer<QPromise<void>>::create();
    for (const Promise<void>& p : ps) {
      p.Then(ctx, [i, qp] {
        (*i)--;
        if (*i == 0) {
          qp->finish();
        }
      });
    }
    return qp->future();
  }

  Promise(const QFuture<void>& another) : QFuture<void>(another) {}

  void Then(QObject* ctx, std::function<void()>&& cb) const {
    if (QFuture<void>::isFinished()) {
      cb();
      return;
    }
    auto w = new QFutureWatcher<void>(ctx);
    QObject::connect(w, &QFutureWatcher<void>::finished, ctx,
                     [cb = std::move(cb), w] {
                       cb();
                       w->deleteLater();
                     });
    w->setFuture(*this);
  }

  template <typename D>
  Promise<D> Then(QObject* ctx, std::function<QFuture<D>()>&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    if (QFuture<void>::isFinished()) {
      Promise<D> d = cb();
      d.Then(ctx, [qpd](D d) {
        qpd->addResult(std::move(d));
        qpd->finish();
      });
      return qpd->future();
    }
    auto w = new QFutureWatcher<void>(ctx);
    QObject::connect(w, &QFutureWatcher<void>::finished, ctx,
                     [cb = std::move(cb), qpd, w, ctx]() {
                       Promise<D> d = cb();
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

#endif  // PROMISE_H
