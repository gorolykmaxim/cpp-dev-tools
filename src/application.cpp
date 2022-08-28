#include "Application.hpp"

Application::Application(int argc, char** argv)
    : gui_app(argc, argv), gui_engine(), runtime(*this), threads(gui_engine) {
  qSetMessagePattern("%{time yyyy-MM-dd h:mm:ss.zzz} %{message}");
  gui_engine.load(QUrl(QStringLiteral("qrc:/cdt/qml/main.qml")));
}
