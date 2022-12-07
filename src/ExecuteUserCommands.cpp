#include "ExecuteUserCommands.hpp"
#include "Process.hpp"

ExecuteUserCommands::ExecuteUserCommands() {
  EXEC_NEXT(ListenToEvents);
}

void ExecuteUserCommands::ListenToEvents(AppData& app) {
  for (const QString& event_type: app.user_commands.keys()) {
    WakeUpProcessOnEvent(app, event_type, *this);
  }
  EXEC_NEXT(Execute);
}

void ExecuteUserCommands::Execute(AppData& app) {
  EXEC_NEXT_DEFERRED(Execute);
  QString event_type = app.events.head().type;
  if (!app.user_commands.contains(event_type)) {
    return;
  }
  app.user_commands[event_type].callback();
}
