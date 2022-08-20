#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "cdt.h"

class MyProcess: public Process {
public:
  MyProcess() {
    execute = Process::kNoopExecute;
  }
};

int main(int argc, char** argv) {
  QGuiApplication q_app(argc, argv);
  QQmlApplicationEngine engine;
  engine.load(QUrl(QStringLiteral("qrc:/cdt/qml/main.qml")));
  Application app;
  app.runtime.Schedule(new MyProcess());
  app.runtime.Schedule(new MyProcess());
  app.runtime.ScheduleAndExecute(new MyProcess());
  app.runtime.Schedule(new MyProcess());
  app.runtime.Schedule(new MyProcess());
  app.runtime.Schedule(new MyProcess());
  app.runtime.ScheduleAndExecute(new MyProcess());
  return q_app.exec();
}
