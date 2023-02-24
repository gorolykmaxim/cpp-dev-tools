#include "Application.hpp"

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
}

int Application::Exec() {
  QtConcurrent::run(&io_thread_pool, &Database::Initialize).waitForFinished();
  qml_engine.load(QUrl("qrc:/cdt/qml/main.qml"));
  return gui_app.exec();
}

void Application::RunIOTask(const std::function<void()>& on_io_thread,
                            const std::function<void()>& on_ui_thread) {
  (void)QtConcurrent::run(&io_thread_pool, [on_io_thread, on_ui_thread]() {
    on_io_thread();
    QMetaObject::invokeMethod(QGuiApplication::instance(),
                              [on_ui_thread]() { on_ui_thread(); });
  });
}

Application* Application::instance;
