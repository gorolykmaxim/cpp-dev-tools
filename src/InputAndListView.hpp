#pragma once

#include "Base.hpp"
#include "UserInterface.hpp"

void InputAndListViewDisplay(
    const QString& input_label, const QString& input_value,
    const QString& button_text, const QVector<QVariantList>& list_items,
    const std::function<void(const QString&)>& on_input_value_changed,
    const std::function<void()>& on_enter_pressed,
    const std::function<void(int)>& on_item_selected, UserInterface& ui);
void InputAndListViewSetItems(const QVector<QVariantList>& list_items,
                              UserInterface& ui);
void InputAndListViewSetInput(const QString& value, UserInterface& ui);
void InputAndListViewSetButtonText(const QString& value, UserInterface& ui);
