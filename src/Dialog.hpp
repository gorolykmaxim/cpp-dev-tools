#pragma once

#include "UserInterface.hpp"

void DialogInit(UserInterface& ui);
void DialogDisplayError(const QString& title, const QString& text,
                        UserInterface& ui);
