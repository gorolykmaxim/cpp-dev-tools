#include "SearchUserCommands.hpp"
#include "Process.hpp"

SearchUserCommands::SearchUserCommands() {
  EXEC_NEXT(DisplaySearchDialog);
}

void SearchUserCommands::DisplaySearchDialog(AppData&) {
  qDebug() << "Displaying user commands search dialog";
  EXEC_NEXT(KeepAlive);
}
