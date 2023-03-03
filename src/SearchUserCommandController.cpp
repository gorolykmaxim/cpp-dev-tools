#include "SearchUserCommandController.hpp"

#include "Application.hpp"
#include "QVariantListModel.hpp"
#include "UserCommandController.hpp"
#include "UserCommandListModel.hpp"

#define LOG() qDebug() << "[SearchUserCommandController"

SearchUserCommandController::SearchUserCommandController(QObject* parent)
    : QObject(parent),
      user_commands(new SimpleQVariantListModel(
          this, {{0, "title"}, {1, "subTitle"}, {2, "rightText"}, {3, "index"}},
          {0, 1})) {}

void SearchUserCommandController::LoadUserCommands() {
  LOG() << "Loadnig user commands";
  this->user_commands->SetFilterIfChanged("");
  this->user_commands->list.clear();
  Application& app = Application::Get();
  UserCommandController& controller = app.user_command_controller;
  const QList<UserCommand>& commands = controller.GetUserCommands();
  for (int i = 0; i < commands.size(); i++) {
    const UserCommand& cmd = commands[i];
    this->user_commands->list.append(
        {cmd.name, cmd.group, cmd.GetFormattedShortcut(), i});
  }
  this->user_commands->Load();
}

void SearchUserCommandController::ExecuteCommand(int i) {
  emit commandExecuted();
  Application::Get().user_command_controller.ExecuteCommand(i);
}
