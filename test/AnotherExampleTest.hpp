#include <QTest>

class AnotherExampleTest : public QObject {
  Q_OBJECT
 private slots:
  void exampleTest() { QVERIFY(true); }
  void anotherTest() { QCOMPARE(1, 1); }
};
