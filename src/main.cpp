#include "AppData.hpp"
#include "Initialize.hpp"
#include "Process.hpp"

int main(int argc, char** argv) {
  AppData app(argc, argv);
  ScheduleProcess<Initialize>(app, nullptr);
  ExecuteProcesses(app);
  return app.gui_app.exec();
}
