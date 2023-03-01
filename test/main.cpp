#include <QGuiApplication>
#include <QTest>

#include "AnotherExampleTest.hpp"
#include "ExampleTest.hpp"

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  int status = 0;
  status |= QTest::qExec(new ExampleTest(), argc, argv);
  status |= QTest::qExec(new AnotherExampleTest(), argc, argv);
  return status;
}
