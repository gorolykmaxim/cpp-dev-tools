#include "ExecUserCmds.hpp"
#include "Process.hpp"

ExecUserCmds::ExecUserCmds() {
  EXEC_NEXT(ListenToEvents);
}

void ExecUserCmds::ListenToEvents(AppData& app) {
  for (const QString& event_type: app.user_cmds.keys()) {
    WakeUpProcessOnEvent(app, event_type, *this);
  }
  EXEC_NEXT(Execute);
}

void ExecUserCmds::Execute(AppData& app) {
  EXEC_NEXT_DEFERRED(Execute);
  QString event_type = app.events.head().type;
  if (!app.user_cmds.contains(event_type)) {
    return;
  }
  app.user_cmds[event_type].callback();
}
