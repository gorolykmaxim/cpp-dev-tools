#include "Application.hpp"
#include <QQmlContext>

Application::Application(int argc, char** argv)
    : gui_app(argc, argv),
      gui_engine(),
      runtime(*this),
      threads(gui_engine),
      ui() {
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
  gui_engine.rootContext()->setContextProperty("ui", &ui);
  gui_engine.load(QUrl(QStringLiteral("qrc:/cdt/qml/main.qml")));
}
