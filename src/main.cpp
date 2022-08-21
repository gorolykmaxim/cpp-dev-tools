#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "cdt.h"

class LongProcess: public Process {
public:
  LongProcess(): my_own_data(15) {
    EXEC_NEXT(DoStuff);
  }
private:
  void DoStuff(Application& app) {
    QSharedPointer<LongProcess> self = app.runtime.SharedPtr(this);
    qDebug() << "Doing stuff" << self->id << self->my_own_data
             << app.runtime.IsAlive(*self);
    EXEC_NEXT(DoOtherStuff);
  }

  void DoOtherStuff(Application& app) {
    qDebug() << "Doing other stuff";
  }

  int my_own_data;
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
  QSharedPointer<LongProcess> p = app.runtime.SharedPtr(proc);
  app.runtime.WakeUpAndExecute(*proc);
  qDebug() << "Is process still alive -" << app.runtime.IsAlive(*p);
  return q_app.exec();
}
