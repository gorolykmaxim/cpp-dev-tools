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
  view.DetermineWindowDimensions();
  editor.Initialize();
  qml_engine.load(QUrl("qrc:/cdt/qml/main.qml"));
  return gui_app.exec();
}

void Application::RunIOTask(QObject* requestor,
                            const std::function<void()>& on_io_thread,
                            const std::function<void()>& on_ui_thread) {
  auto watcher = new QFutureWatcher<void>();
  QObject::connect(watcher, &QFutureWatcher<void>::finished, requestor,
                   [watcher, on_ui_thread]() {
                     on_ui_thread();
                     watcher->deleteLater();
                   });
  QFuture<void> future = QtConcurrent::run(&io_thread_pool, on_io_thread);
  watcher->setFuture(future);
}

Application* Application::instance;
