#include "search_user_command_controller.h"

#include "application.h"
#include "text_list_model.h"
#include "user_command_system.h"

#define LOG() qDebug() << "[SearchUserCommandController]"

SearchUserCommandController::SearchUserCommandController(QObject* parent)
    : QObject(parent), user_commands(new SearchUserCommandListModel(this)) {}

void SearchUserCommandController::loadUserCommands() {
  LOG() << "Loading user commands";
  if (!user_commands->SetFilterIfChanged("")) {
    user_commands->Load();
  }
}

void SearchUserCommandController::executeSelectedCommand() {
  emit commandExecuted();
  int i = user_commands->GetSelectedItemIndex();
  Application::Get().user_command.executeCommand(i);
}

SearchUserCommandListModel::SearchUserCommandListModel(QObject* parent)
    : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "rightText"}});
  searchable_roles = {0};
}

QVariantList SearchUserCommandListModel::GetRow(int i) const {
  const UserCommand& cmd = Application::Get().user_command.GetUserCommands()[i];
  QString title = cmd.group + ": " + cmd.name;
  return {title, cmd.GetFormattedShortcut()};
}

int SearchUserCommandListModel::GetRowCount() const {
  return Application::Get().user_command.GetUserCommands().size();
}
