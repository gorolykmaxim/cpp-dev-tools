#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QThreadPool>
#include <QtConcurrent>

#include "Database.hpp"

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;
  QThreadPool io_thread_pool;
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
  QtConcurrent::run(&io_thread_pool, &Database::Initialize).waitForFinished();
  engine.load(QUrl("qrc:/cdt/qml/main.qml"));
  return app.exec();
}
