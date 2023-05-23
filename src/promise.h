#ifndef PROMISE_H
#define PROMISE_H

#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>
#include <QSharedPointer>
#include <type_traits>

template <typename T>
class Promise : public QFuture<T> {
  template <class U>
  using DisableForVoid = std::enable_if_t<!std::is_same_v<U, void>, bool>;
  template <class U>
  using EnableForVoid = std::enable_if_t<std::is_same_v<U, void>, bool>;

 public:
  template <typename... Promises>
  static Promise<void> All(QObject* ctx, Promises&&... args) {
    QList<Promise<void>> ps = {args...};
    auto i = QSharedPointer<int>::create(ps.size());
    auto qp = QSharedPointer<QPromise<void>>::create();
    for (const Promise<void>& p : ps) {
      p.Then(ctx, [i, qp]() {
        (*i)--;
        if (*i == 0) {
          qp->finish();
        }
      });
    }
    return qp->future();
  }

  Promise() : QFuture<T>() {}

  template <typename V, typename U = T, DisableForVoid<U> = true>
  explicit Promise(const V& v) : QFuture<T>(QtFuture::makeReadyFuture(v)) {}

  template <typename V, typename U = T, DisableForVoid<U> = true>
  explicit Promise(V&& v) : QFuture<T>(QtFuture::makeReadyFuture(v)) {}

  Promise(const QFuture<T>& another) : QFuture<T>(another) {}

  template <class Function, typename U = T, DisableForVoid<U> = true>
  void Then(QObject* ctx, Function&& cb) const {
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

  template <class Function, typename U = T, EnableForVoid<U> = true>
  void Then(QObject* ctx, Function&& cb) const {
    if (QFuture<T>::isFinished()) {
      cb();
      return;
    }
    auto w = new QFutureWatcher<T>(ctx);
    QObject::connect(w, &QFutureWatcher<T>::finished, ctx,
                     [cb = std::move(cb), w] {
                       cb();
                       w->deleteLater();
                     });
    w->setFuture(*this);
  }

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<!std::is_same_v<U, void> && !std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
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

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<!std::is_same_v<U, void> && std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    if (QFuture<T>::isFinished()) {
      Promise<D> d = cb(QFuture<T>::result());
      d.Then(ctx, [qpd]() { qpd->finish(); });
      return qpd->future();
    }
    auto w = new QFutureWatcher<T>(ctx);
    QObject::connect(w, &QFutureWatcher<T>::finished, ctx,
                     [cb = std::move(cb), qpd, w, ctx]() {
                       Promise<D> d = cb(w->result());
                       d.Then(ctx, [qpd]() { qpd->finish(); });
                       w->deleteLater();
                     });
    w->setFuture(*this);
    return qpd->future();
  }

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<std::is_same_v<U, void> && !std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    if (QFuture<T>::isFinished()) {
      Promise<D> d = cb();
      d.Then(ctx, [qpd](D d) {
        qpd->addResult(std::move(d));
        qpd->finish();
      });
      return qpd->future();
    }
    auto w = new QFutureWatcher<T>(ctx);
    QObject::connect(w, &QFutureWatcher<T>::finished, ctx,
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

  template <typename D, class Function, typename U = T,
            std::enable_if_t<std::is_same_v<U, void> && std::is_same_v<D, void>,
                             bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    if (QFuture<T>::isFinished()) {
      Promise<D> d = cb();
      d.Then(ctx, [qpd]() { qpd->finish(); });
      return qpd->future();
    }
    auto w = new QFutureWatcher<T>(ctx);
    QObject::connect(w, &QFutureWatcher<T>::finished, ctx,
                     [cb = std::move(cb), qpd, w, ctx]() {
                       Promise<D> d = cb();
                       d.Then(ctx, [qpd]() { qpd->finish(); });
                       w->deleteLater();
                     });
    w->setFuture(*this);
    return qpd->future();
  }
};

#endif  // PROMISE_H
