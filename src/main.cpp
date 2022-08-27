#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include "application.hpp"

class LongProcess: public Process {
public:
  LongProcess(int my_own_data): my_own_data(my_own_data) {
    EXEC_NEXT(DoStuff);
  }
private:
  void DoStuff(Application& app) {
    QPtr<LongProcess> self = app.runtime.SharedPtr(this);
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
  Application app(engine);
  // Test ProcessRuntime
  app.runtime.ScheduleRoot<MyProcess>();
  app.runtime.ScheduleRoot<MyProcess>();
  app.runtime.ScheduleAndExecute<MyProcess>();
  app.runtime.ScheduleRoot<MyProcess>();
  app.runtime.ScheduleRoot<MyProcess>();
  app.runtime.ScheduleRoot<MyProcess>();
  app.runtime.ScheduleAndExecute<MyProcess>();
  QPtr<LongProcess> proc = app.runtime.ScheduleAndExecute<LongProcess>(32);
  QPtr<LongProcess> p = app.runtime.SharedPtr(proc.get());
  app.runtime.WakeUpAndExecute(*proc);
  qDebug() << "Is process still alive -" << app.runtime.IsAlive(*p);
  // Test Threads
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
