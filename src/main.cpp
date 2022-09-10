#include <QThread>
#include "Application.hpp"
#include "Initialize.hpp"

int main(int argc, char** argv) {
  Application app(argc, argv);
  app.runtime.ScheduleAndExecuteRoot<Initialize>();
  return app.gui_app.exec();
}
