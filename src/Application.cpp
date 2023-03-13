#include "Application.hpp"

#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>

#include "Database.hpp"

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
#if defined(_WIN32) && defined(NDEBUG)
  // This fixes laggy window movement on an external monitor on Windows and
  // slightly slower selection in TextArea.
  // Although we don't want to do it in a debug build because in debug software
  // rendering is even slower.
  QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
#endif
  qml_engine.rootContext()->setContextProperty("projectSystem", &project);
  qml_engine.rootContext()->setContextProperty("viewSystem", &view_controller);
  qml_engine.rootContext()->setContextProperty("userCommandSystem",
                                               &user_command_controller);
#if __APPLE__
  qml_engine.rootContext()->setContextProperty("isMacOS", true);
#else
  qml_engine.rootContext()->setContextProperty("isMacOS", false);
#endif
}

Application::~Application() { task.KillAllTasks(); }

int Application::Exec() {
  QtConcurrent::run(&io_thread_pool, &Database::Initialize).waitForFinished();
  user_command_controller.RegisterCommands();
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
