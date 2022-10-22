#pragma once

#include "Base.hpp"
#include "UserInterface.hpp"

void InputAndListViewDisplay(
    UserInterface& ui, const QString& input_label, const QString& input_value,
    const QString& button_text, const QVector<QVariantList>& list_items,
    const std::function<void(const QString&)>& on_input_value_changed,
    const std::function<void()>& on_enter_pressed,
    const std::function<void(int)>& on_item_selected);
void InputAndListViewSetItems(UserInterface& ui,
                              const QVector<QVariantList>& list_items);
void InputAndListViewSetInput(UserInterface& ui, const QString& value);
void InputAndListViewSetButtonText(UserInterface& ui, const QString& value);
