#include "Application.hpp"
#include "Initialize.hpp"

int main(int argc, char** argv) {
  Application app(argc, argv);
  app.runtime.ScheduleAndExecute<Initialize>(nullptr);
  return app.gui_app.exec();
}
