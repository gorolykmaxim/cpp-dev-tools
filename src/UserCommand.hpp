#pragma once

#include <QString>
#include <functional>

struct UserCommand {
  QString GetFormattedShortcut() const;

  QString group;
  QString name;
  QString shortcut;
  std::function<void()> callback;
};
