#include "AppData.hpp"
#include "Initialize.hpp"
#include "Process.hpp"

int main(int argc, char** argv) {
  AppData app(argc, argv);
  ScheduleAndExecuteProcess<Initialize>(app, nullptr);
  return app.gui_app.exec();
}
