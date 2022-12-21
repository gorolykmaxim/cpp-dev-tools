#include "CloseProject.hpp"
#include "Process.hpp"
#include "SelectProject.hpp"

CloseProject::CloseProject() {
  EXEC_NEXT(Close);
}

void CloseProject::Close(AppData& app) {
  app.profiles.clear();
  app.task_defs.clear();
  app.tasks.clear();
  app.current_project_path = "";
  ScheduleProcess<SelectProject>(app, kViewSlot);
}
