#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "cdt.h"

class LongProcess: public Process {
public:
  LongProcess() {
    EXEC_NEXT(DoStuff);
  }
private:
  void DoStuff(Application& app) {
    qDebug() << "Doing stuff";
    EXEC_NEXT(DoOtherStuff);
  }

  void DoOtherStuff(Application& app) {
    qDebug() << "Doing other stuff";
  }
};

class MyProcess: public Process {
public:
  MyProcess() {
    EXEC_NEXT(Process::kNoopExecute);
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
  LongProcess *proc = new LongProcess();
  app.runtime.ScheduleAndExecute(proc);
  app.runtime.WakeUpAndExecute(*proc);
  return q_app.exec();
}
