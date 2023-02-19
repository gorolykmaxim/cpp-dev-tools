#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;
  QQuickStyle::setStyle("Basic");
  engine.load(QUrl("qrc:/cdt/qml/main.qml"));
  return app.exec();
}
