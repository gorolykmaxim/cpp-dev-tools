#include "Application.hpp"

int main(int argc, char** argv) {
  auto app = QSharedPointer<Application>::create(argc, argv);
  Application::instance = app.get();
  return app->Exec();
}
