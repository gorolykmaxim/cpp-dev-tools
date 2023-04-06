#include <QGuiApplication>
#include <QTest>

#include "another_example_test.h"
#include "example_test.h"

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  int status = 0;
  status |= QTest::qExec(new ExampleTest(), argc, argv);
  status |= QTest::qExec(new AnotherExampleTest(), argc, argv);
  return status;
}
