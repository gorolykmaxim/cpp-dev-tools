#include "Application.hpp"

#include <QQuickStyle>

#include "Database.hpp"

Application& Application::Get() { return *instance; }

Application::Application(int argc, char** argv)
    : gui_app(argc, argv), qml_engine(), io_thread_pool() {
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

Application* Application::instance;
