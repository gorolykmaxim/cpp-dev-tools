#include "search_user_command_controller.h"

#include "application.h"
#include "qvariant_list_model.h"
#include "user_command_system.h"

#define LOG() qDebug() << "[SearchUserCommandController]"

SearchUserCommandController::SearchUserCommandController(QObject* parent)
    : QObject(parent),
      user_commands(new SimpleQVariantListModel(
          this, {{0, "title"}, {1, "subTitle"}, {2, "rightText"}, {3, "index"}},
          {0, 1})) {}

void SearchUserCommandController::loadUserCommands() {
  LOG() << "Loading user commands";
  user_commands->SetFilterIfChanged("");
  user_commands->list.clear();
  Application& app = Application::Get();
  const QList<UserCommand>& commands = app.user_command.GetUserCommands();
  for (int i = 0; i < commands.size(); i++) {
    const UserCommand& cmd = commands[i];
    user_commands->list.append(
        {cmd.name, cmd.group, cmd.GetFormattedShortcut(), i});
  }
  user_commands->Load();
}

void SearchUserCommandController::executeCommand(int i) {
  emit commandExecuted();
  Application::Get().user_command.executeCommand(i);
}
