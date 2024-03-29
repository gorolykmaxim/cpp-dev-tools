#include "application.h"

#include <QFontDatabase>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>

#include "database.h"
#include "os_command.h"

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
  QString fonts = ":/cdt/fonts/";
  for (const QString& font : QDir(fonts).entryList()) {
    int e = QFontDatabase::addApplicationFont(fonts + font);
    Q_ASSERT(e >= 0);
  }
  qml_engine.rootContext()->setContextProperty("projectSystem", &project);
  qml_engine.rootContext()->setContextProperty("viewSystem", &view);
  qml_engine.rootContext()->setContextProperty("userCommandSystem",
                                               &user_command);
  qml_engine.rootContext()->setContextProperty("taskSystem", &task);
  qml_engine.rootContext()->setContextProperty("editorSystem", &editor);
  qml_engine.rootContext()->setContextProperty("notificationSystem",
                                               &notification);
  qml_engine.rootContext()->setContextProperty("documentationSystem",
                                               &documentation);
  qml_engine.rootContext()->setContextProperty("sqliteSystem", &sqlite);
  qml_engine.rootContext()->setContextProperty("gitSystem", &git);
#if __APPLE__
  qml_engine.rootContext()->setContextProperty("monoFontFamily", "Menlo");
  qml_engine.rootContext()->setContextProperty("monoFontSize", 12);
  OsCommand::LoadEnvironmentVariablesOnMac(&gui_app);
#else
  qml_engine.rootContext()->setContextProperty("monoFontFamily", "Consolas");
  qml_engine.rootContext()->setContextProperty("monoFontSize", 10);
  gui_app.setWindowIcon(QIcon(":/cdt/win/cpp-dev-tools.ico"));
#endif
}

Application::~Application() { task.KillAllTasks(); }

int Application::Exec() {
  QtConcurrent::run(&io_thread_pool, &Database::Initialize).waitForFinished();
  user_command.Initialize();
  view.Initialize();
  editor.Initialize();
  task.Initialize();
  OsCommand::InitTerminals();
  qml_engine.load(QUrl("qrc:/cdt/qml/main.qml"));
  return gui_app.exec();
}

Application* Application::instance;
