#include "search_user_command_controller.h"

#include "application.h"
#include "text_list_model.h"
#include "user_command_system.h"

#define LOG() qDebug() << "[SearchUserCommandController]"

SearchUserCommandController::SearchUserCommandController(QObject* parent)
    : QObject(parent),
      user_commands(new SimpleTextListModel(
          this, {{0, "title"}, {1, "rightText"}, {2, "index"}}, {0})) {}

void SearchUserCommandController::loadUserCommands() {
  LOG() << "Loading user commands";
  user_commands->SetFilterIfChanged("");
  user_commands->list.clear();
  Application& app = Application::Get();
  const QList<UserCommand>& commands = app.user_command.GetUserCommands();
  for (int i = 0; i < commands.size(); i++) {
    const UserCommand& cmd = commands[i];
    QString title = cmd.group + ": " + cmd.name;
    user_commands->list.append({title, cmd.GetFormattedShortcut(), i});
  }
  user_commands->Load();
}

void SearchUserCommandController::executeCommand(int i) {
  emit commandExecuted();
  Application::Get().user_command.executeCommand(i);
}
