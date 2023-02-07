#include "CloseProject.hpp"

#include "Db.hpp"
#include "Process.hpp"
#include "SelectProject.hpp"

CloseProject::CloseProject() { EXEC_NEXT(Close); }

void CloseProject::Close(AppData& app) {
  app.current_project = nullptr;
  app.execs.clear();
  ExecDbCmdOnIOThread(app, "UPDATE project SET is_opened=false");
  ScheduleProcess<SelectProject>(app, kViewSlot);
}
