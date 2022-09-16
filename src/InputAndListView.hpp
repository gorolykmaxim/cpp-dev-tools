#pragma once

#include <functional>
#include <QString>
#include <QVariant>
#include <QVector>
#include "UserInterface.hpp"

class InputAndListView {
public:
  static void Display(
      const QString& input_label, const QString& input_value,
      const QString& button_text, const QVector<QVariantList>& list_items,
      const std::function<void(const QString&)>& on_input_value_changed,
      const std::function<void()>& on_enter_pressed,
      const std::function<void(int)>& on_item_selected, UserInterface& ui);
  static void SetItems(const QVector<QVariantList>& list_items,
                       UserInterface& ui);
  static void SetInput(const QString& value, UserInterface& ui);
  static void SetButtonText(const QString& value, UserInterface& ui);
};
