#include "UserCommand.hpp"

QString UserCommand::GetFormattedShortcut() const {
  QString result = shortcut.toUpper();
#if __APPLE__
  result.replace("CTRL", "\u2318");
#endif
  return result;
}
