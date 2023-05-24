#ifndef PROMISE_H
#define PROMISE_H

#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>
#include <QSharedPointer>
#include <type_traits>

template <typename T>
class Promise : public QFuture<T> {
 public:
  Promise() : QFuture<T>() {}

  template <typename V, typename U = T,
            std::enable_if_t<!std::is_same_v<U, void>, bool> = true>
  explicit Promise(const V& v) : QFuture<T>(QtFuture::makeReadyFuture(v)) {}

  template <typename V, typename U = T,
            std::enable_if_t<!std::is_same_v<U, void>, bool> = true>
  explicit Promise(V&& v) : QFuture<T>(QtFuture::makeReadyFuture(v)) {}

  Promise(const QFuture<T>& another) : QFuture<T>(another) {}

  Promise(QFuture<T>&& another) : QFuture<T>(another) {}

  template <class Function, typename U = T,
            std::enable_if_t<!std::is_same_v<U, void>, bool> = true>
  void Then(QObject* ctx, Function&& cb) const {
    OnFinished(
        ctx, [cb = std::move(cb)](const Promise<T>& f) { cb(f.result()); },
        [] {});
  }

  template <class Function, typename U = T,
            std::enable_if_t<std::is_same_v<U, void>, bool> = true>
  void Then(QObject* ctx, Function&& cb) const {
    OnFinished(
        ctx, [cb = std::move(cb)](const Promise<T>&) { cb(); }, [] {});
  }

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<!std::is_same_v<U, void> && !std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    OnFinished(
        ctx,
        [cb = std::move(cb), qpd, ctx](const Promise<T>& f) {
          Promise<D> d = cb(f.result());
          d.Then(ctx, [qpd](D d) {
            qpd->addResult(std::move(d));
            qpd->finish();
          });
        },
        [qpd] { qpd->future().cancel(); });
    return qpd->future();
  }

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<!std::is_same_v<U, void> && std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    OnFinished(
        ctx,
        [cb = std::move(cb), qpd, ctx](const Promise<T>& f) {
          Promise<D> d = cb(f.result());
          d.Then(ctx, [qpd]() { qpd->finish(); });
        },
        [qpd] { qpd->future().cancel(); });
    return qpd->future();
  }

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<std::is_same_v<U, void> && !std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    OnFinished(
        ctx,
        [cb = std::move(cb), qpd, ctx](const Promise<T>&) {
          Promise<D> d = cb();
          d.Then(ctx, [qpd](D d) {
            qpd->addResult(std::move(d));
            qpd->finish();
          });
        },
        [qpd] { qpd->future().cancel(); });
    return qpd->future();
  }

  template <typename D, class Function, typename U = T,
            std::enable_if_t<std::is_same_v<U, void> && std::is_same_v<D, void>,
                             bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    OnFinished(
        ctx,
        [cb = std::move(cb), qpd, ctx](const Promise<T>&) {
          Promise<D> d = cb();
          // TODO: if this promise represents a chain in and of itself (we are
          // inside Then() cb and we create a brand new Then() chain inside and
          // return its result. If one of the Then() from that chain cancels -
          // the promise we have here will get cancelled asynchronously. BUT we
          // won't cancel our QPromise and leak it and all that is hanging on
          // it.
          d.Then(ctx, [qpd]() { qpd->finish(); });
        },
        [qpd] { qpd->future().cancel(); });
    return qpd->future();
  }

 private:
  void OnFinished(QObject* ctx, std::function<void(const Promise<T>&)>&& cb,
                  std::function<void()>&& cancel_cb) const {
    if (QFuture<T>::isCanceled()) {
      cancel_cb();
    } else if (QFuture<T>::isFinished()) {
      cb(*this);
    } else {
      auto w = new QFutureWatcher<T>(ctx);
      QObject::connect(
          w, &QFutureWatcher<T>::finished, ctx,
          [cb = std::move(cb), cancel_cb = std::move(cancel_cb), w]() {
            if (w->isCanceled()) {
              cancel_cb();
            } else {
              cb(Promise<T>(w->future()));
            }
            w->deleteLater();
          });
      w->setFuture(*this);
    }
  }

  friend class Promises;
};

class Promises {
 public:
  template <typename D, typename... Promises>
  static Promise<QList<Promise<D>>> All(QObject* ctx, Promises&&... args) {
    QList<Promise<D>> ps = {args...};
    auto i = QSharedPointer<int>::create(ps.size());
    auto data = QSharedPointer<QList<Promise<D>>>::create();
    auto qp = QSharedPointer<QPromise<QList<Promise<D>>>>::create();
    for (const Promise<D>& p : ps) {
      p.OnFinished(
          ctx,
          [i, qp, data](const Promise<D>& p) {
            (*i)--;
            data->append(p);
            if (*i == 0) {
              qp->addResult(*data);
              qp->finish();
            }
          },
          [i, data, p] {
            (*i)--;
            data->append(p);
          });
    }
    return qp->future();
  }
};

#endif  // PROMISE_H
