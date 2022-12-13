#include "SearchUserCommands.hpp"
#include "Process.hpp"

SearchUserCommands::SearchUserCommands() {
  EXEC_NEXT(DisplaySearchDialog);
  ON_CANCEL(Cancel);
}

void SearchUserCommands::DisplaySearchDialog(AppData&) {
  qDebug() << "Displaying user commands search dialog";
  EXEC_NEXT(KeepAlive);
}

void SearchUserCommands::Cancel(AppData&) {
  qDebug() << "Closing dialog window";
}
