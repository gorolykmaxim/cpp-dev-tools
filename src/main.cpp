#include "Application.hpp"
#include "Initialize.hpp"
#include "Process.hpp"

int main(int argc, char** argv) {
  Application app(argc, argv);
  ScheduleAndExecuteProcess<Initialize>(app, nullptr);
  return app.gui_app.exec();
}
