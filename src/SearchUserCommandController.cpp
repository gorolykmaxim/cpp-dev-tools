#include "SearchUserCommandController.hpp"

#include "QVariantListModel.hpp"
#include "UserCommandListModel.hpp"

#define LOG() qDebug() << "[SearchUserCommandController"

SearchUserCommandController::SearchUserCommandController(QObject* parent)
    : QObject(parent),
      user_commands(new SimpleQVariantListModel(
          this,
          {{0, "title"}, {1, "subTitle"}, {2, "rightText"}, {3, "callback"}},
          {0, 1})) {}

void SearchUserCommandController::LoadUserCommands(
    UserCommandListModel* user_commands) {
  LOG() << "Loadnig user commands";
  this->user_commands->SetFilterIfChanged("");
  this->user_commands->list.clear();
  if (user_commands) {
    for (const UserCommand& cmd : user_commands->list) {
      this->user_commands->list.append(
          {cmd.name, cmd.group, cmd.GetFormattedShortcut(), cmd.callback});
    }
  }
  this->user_commands->Load();
}
