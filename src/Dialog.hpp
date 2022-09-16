#pragma once

#include "UserInterface.hpp"

class Dialog {
public:
  static void Init(UserInterface& ui);
  static void DisplayError(const QString& title, const QString& text,
                           UserInterface& ui);
};
