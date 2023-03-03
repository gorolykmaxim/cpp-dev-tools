#pragma once

#include <QString>
#include <functional>

class UserCommand {
 public:
  QString GetFormattedShortcut() const;

  QString group;
  QString name;
  QString shortcut;
  std::function<void()> callback;
};
