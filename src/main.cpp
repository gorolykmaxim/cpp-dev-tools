#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include "application.h"

int main(int argc, char** argv) {
  QGuiApplication q_app(argc, argv);
  QQmlApplicationEngine engine;
  engine.load(QUrl(QStringLiteral("qrc:/cdt/qml/main.qml")));
  Application app(engine);
  for (int i = 0; i < 10; i++) {
    app.threads.ScheduleIO<int>([=] () {
      qDebug() << QThread::currentThreadId() << i << "Running computation";
      return 32 + 15;
    }, [=] (int value) {
      qDebug() << QThread::currentThreadId() << i << value;
    });
  }
  return q_app.exec();
}
