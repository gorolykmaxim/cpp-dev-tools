#include "application.h"

#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>

#include "database.h"

Application& Application::Get() { return *instance; }

Application::Application(int argc, char** argv)
    : argc_(argc),
      argv_(argv),
      gui_app(argc_, argv_),
      qml_engine(),
      io_thread_pool() {
  Application::instance = this;
  io_thread_pool.setMaxThreadCount(1);
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
  QQuickStyle::setStyle("Basic");
  qml_engine.rootContext()->setContextProperty("projectSystem", &project);
  qml_engine.rootContext()->setContextProperty("viewSystem", &view);
  qml_engine.rootContext()->setContextProperty("userCommandSystem",
                                               &user_command);
  qml_engine.rootContext()->setContextProperty("taskSystem", &task);
  qml_engine.rootContext()->setContextProperty("editorSystem", &editor);
  qml_engine.rootContext()->setContextProperty("notificationSystem",
                                               &notification);
#if __APPLE__
  qml_engine.rootContext()->setContextProperty("isMacOS", true);
#else
  qml_engine.rootContext()->setContextProperty("isMacOS", false);
#endif
}

Application::~Application() { task.KillAllTasks(); }

int Application::Exec() {
  QtConcurrent::run(&io_thread_pool, &Database::Initialize).waitForFinished();
  user_command.RegisterCommands();
  view.Initialize();
  editor.Initialize();
  task.Initialize();
  qml_engine.load(QUrl("qrc:/cdt/qml/main.qml"));
  return gui_app.exec();
}

Application* Application::instance;
