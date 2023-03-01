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
 signals:
  void publish(QString data);
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

  void cleanupTestCase() {
    qDebug("Called after myFirstTest and mySecondTest.");
  }
};

QTEST_MAIN(ExampleTest)
#include "ExampleTest.moc"
