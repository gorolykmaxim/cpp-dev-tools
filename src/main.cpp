#include <QThread>
#include "application.hpp"
#include "process/config.hpp"

int main(int argc, char** argv) {
  Application app(argc, argv);
  app.runtime.ScheduleAndExecute<Initialize>();
  return app.gui_app.exec();
}
