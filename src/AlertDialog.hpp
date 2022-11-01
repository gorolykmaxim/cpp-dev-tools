#pragma once

#include "Lib.hpp"
#include "UserInterface.hpp"

void AlertDialogDisplay(UserInterface& ui, const QString& title,
                        const QString& text, bool error = true,
                        bool cancellable = false,
                        const std::function<void()>& on_ok = nullptr,
                        const std::function<void()>& on_cancel = nullptr);
