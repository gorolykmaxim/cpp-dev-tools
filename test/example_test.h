#include <QSignalSpy>
#include <QTest>
#include <QtConcurrent>

class Consumer : public QObject {
  Q_OBJECT
 public slots:
  void consume(QString data) {
    qDebug() << "incoming data" << data;
    (void)QtConcurrent::run([this]() {
      qDebug() << "In background thread";
      QMetaObject::invokeMethod(this, [this]() { emit haveConsumed(); });
    });
    emit haveConsumed();
  }
 signals:
  void haveConsumed();
};

class Publisher : public QObject {
  Q_OBJECT
 public:
  void doStuff() { emit publish("hello world!"); }
  QFuture<QString> doStuffAsync() {
    QTimer::singleShot(50, this, &Publisher::doStuff);
    return QtFuture::connect(this, &Publisher::publish);
  }
 signals:
  void publish(QString data);
};

class SimpleConsumer : public QObject {
  Q_OBJECT
 public slots:
  void consume(QString data) {
    qDebug() << "incoming data" << data;
    emit haveConsumed();
  }
 signals:
  void haveConsumed();
};

class ExampleTest : public QObject {
  Q_OBJECT

 private:
  bool myCondition() { return true; }

 private slots:
  void initTestCase() { qDebug("Called before everything else."); }

  void myFirstTest() {
    QVERIFY(true);   // check that a condition is satisfied
    QCOMPARE(1, 1);  // compare two values
  }

  void mySecondTest() {
    QVERIFY(myCondition());
    QVERIFY(1 != 2);
  }

  void mySignalSlotTest() {
    Publisher p;
    Consumer c;
    QSignalSpy spy(&c, &Consumer::haveConsumed);
    QObject::connect(&p, &Publisher::publish, &c, &Consumer::consume);
    qDebug() << "Test is starting";
    p.doStuff();
    QVERIFY(spy.wait(1000));
    qDebug() << "Test is done";
  }

  void futureAndPromiseTest() {
    SimpleConsumer c;
    QSignalSpy spy(&c, &SimpleConsumer::haveConsumed);
    QtConcurrent::run([] {
      qDebug() << "BACKGROUND" << QThread::currentThread();
      return "hello world";
    })
        .then([](QString data) {
          qDebug() << "FIRST THEN" << QThread::currentThread() << data;
          return data.remove("world").trimmed();
        })
        .then(&c, [&c](QString data) {
          qDebug() << "SECOND THEN" << QThread::currentThread() << data;
          c.consume(data);
        });
    QVERIFY(spy.wait(1000));
    qDebug() << "TEST DONE ON" << QThread::currentThread();
  }

  void futureAndPromiseAsyncTest() {
    Publisher p;
    SimpleConsumer c;
    QSignalSpy spy(&c, &SimpleConsumer::haveConsumed);
    p.doStuffAsync().then([&c](QString data) {
      qDebug() << "FIRST THEN" << QThread::currentThread() << data;
      c.consume(data);
    });
    QVERIFY(spy.wait(1000));
  }

  void futureAndPromiseAsyncDeleteTest() {
    Publisher p;
    SimpleConsumer c1;
    auto c2 = new SimpleConsumer();
    QFutureWatcher<QString> w;
    connect(&w, &QFutureWatcher<QString>::finished, c2, [&c1, c2, &w]() {
      qDebug() << "FIRST THEN" << QThread::currentThread() << w.result() << c2;
      c1.consume(w.result());
    });
    QSignalSpy spy(&c1, &SimpleConsumer::haveConsumed);
    QFuture<QString> f = p.doStuffAsync().then([](QString data) {
      qDebug() << "Pre future watcher" << data;
      return data;
    });
    w.setFuture(f);
    delete c2;
    QVERIFY(!spy.wait(200));
  }

  void futureAndPromiseFutureWatcherDeleteTest() {
    Publisher p;
    SimpleConsumer c;
    auto w = new QFutureWatcher<QString>();
    connect(w, &QFutureWatcher<QString>::finished, &c, [&c, w]() {
      qDebug() << "FIRST THEN" << QThread::currentThread() << w->result();
      c.consume(w->result());
    });
    QSignalSpy spy(&c, &SimpleConsumer::haveConsumed);
    QFuture<QString> f = p.doStuffAsync().then([](QString data) {
      qDebug() << "Pre future watcher" << data;
      return data;
    });
    w->setFuture(f);
    delete w;
    QVERIFY(!spy.wait(200));
  }

  void futureAndPromiseOnMultipleFuturesTest() {
    Publisher p;
    SimpleConsumer c;
    QSignalSpy spy(&c, &SimpleConsumer::haveConsumed);
    QFuture<QString> f1 = p.doStuffAsync();
    QFuture<QString> f2 = p.doStuffAsync();
    QList<QFuture<QString>> fs = {f1, f2};
    QtFuture::whenAll<QList<QFuture<QString>>>(fs.begin(), fs.end())
        .then(&c, [&c](const QList<QFuture<QString>>& fs) {
          for (const QFuture<QString>& f : fs) {
            qDebug() << "IN THEN" << f.result();
          }
          c.consume(fs[0].result());
        });
    QVERIFY(spy.wait(200));
  }

  void cleanupTestCase() {
    qDebug("Called after myFirstTest and mySecondTest.");
  }
};
