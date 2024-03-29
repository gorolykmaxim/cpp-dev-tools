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

  /**
   * When a callback inside Then() returns Promise<T>() (which is cancelled) -
   * the whole chain of Thens after it gets cancelled (their callbacks don't get
   * executed). Though the QFutureWatchers allocated inside downstream Thens do
   * get de-allocated: When our QFutureWatcher gets its finished() triggered by
   * a cancelled future, we don't trigger our callback and just return. When we
   * return - we de-allocate the last reference to our QPromise, which triggers
   * it's destruction. When QPromise gets deleted its destructor cancels the
   * underlying future, thus all downstream Then() will have their
   * QFutureWatcher::finished triggered by this "destruction" cancel.
   */
  template <class Function, typename U = T,
            std::enable_if_t<!std::is_same_v<U, void>, bool> = true>
  void Then(QObject* ctx, Function&& cb) const {
    OnFinished(ctx,
               [cb = std::move(cb)](const Promise<T>& f) { cb(f.result()); });
  }

  template <class Function, typename U = T,
            std::enable_if_t<std::is_same_v<U, void>, bool> = true>
  void Then(QObject* ctx, Function&& cb) const {
    OnFinished(ctx, [cb = std::move(cb)](const Promise<T>&) { cb(); });
  }

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<!std::is_same_v<U, void> && !std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    OnFinished(ctx, [cb = std::move(cb), qpd, ctx](const Promise<T>& f) {
      Promise<D> d = cb(f.result());
      d.Then(ctx, [qpd](D d) {
        qpd->addResult(std::move(d));
        qpd->finish();
      });
    });
    return qpd->future();
  }

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<!std::is_same_v<U, void> && std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    OnFinished(ctx, [cb = std::move(cb), qpd, ctx](const Promise<T>& f) {
      Promise<D> d = cb(f.result());
      d.Then(ctx, [qpd]() { qpd->finish(); });
    });
    return qpd->future();
  }

  template <
      typename D, class Function, typename U = T,
      std::enable_if_t<std::is_same_v<U, void> && !std::is_same_v<D, void>,
                       bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    OnFinished(ctx, [cb = std::move(cb), qpd, ctx](const Promise<T>&) {
      Promise<D> d = cb();
      d.Then(ctx, [qpd](D d) {
        qpd->addResult(std::move(d));
        qpd->finish();
      });
    });
    return qpd->future();
  }

  template <typename D, class Function, typename U = T,
            std::enable_if_t<std::is_same_v<U, void> && std::is_same_v<D, void>,
                             bool> = true>
  Promise<D> Then(QObject* ctx, Function&& cb) const {
    auto qpd = QSharedPointer<QPromise<D>>::create();
    OnFinished(ctx, [cb = std::move(cb), qpd, ctx](const Promise<T>&) {
      Promise<D> d = cb();
      d.Then(ctx, [qpd]() { qpd->finish(); });
    });
    return qpd->future();
  }

 private:
  void OnFinished(QObject* ctx, std::function<void(const Promise<T>&)>&& cb,
                  bool cb_on_cancel = false) const {
    if (QFuture<T>::isFinished()) {
      if (!QFuture<T>::isCanceled() || cb_on_cancel) {
        cb(*this);
      }
    } else {
      auto w = new QFutureWatcher<T>(ctx);
      QObject::connect(w, &QFutureWatcher<T>::finished, ctx,
                       [cb = std::move(cb), cb_on_cancel, w]() {
                         if (!w->isCanceled() || cb_on_cancel) {
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
          true);
    }
    return qp->future();
  }
};

#endif  // PROMISE_H
