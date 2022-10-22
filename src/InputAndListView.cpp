#include "InputAndListView.hpp"

void InputAndListViewDisplay(
    UserInterface& ui, const QString& input_label, const QString& input_value,
    const QString& button_text, const QVector<QVariantList>& list_items,
    const std::function<void(const QString&)>& on_input_value_changed,
    const std::function<void()>& on_enter_pressed,
    const std::function<void(int)>& on_item_selected) {
  UserActionHandler input_handler = [on_input_value_changed] (const QVariantList& args) {
    on_input_value_changed(args[0].toString());
  };
  UserActionHandler enter_handler = [on_enter_pressed] (const QVariantList&) {
    on_enter_pressed();
  };
  UserActionHandler item_handler = [on_item_selected] (const QVariantList& args) {
    on_item_selected(args[0].toInt());
  };
  ui.DisplayView(
      kViewSlot,
      "InputAndListView.qml",
      {
        DataField{"dataViewInputLabel", input_label},
        DataField{"dataViewInputValue", input_value},
        DataField{"dataViewButtonText", button_text},
      },
      {
        ListField{"dataViewListModel", {{0, "title"}}, list_items},
      },
      {
        {"actionViewInputValueChanged", input_handler},
        {"actionViewEnterPressed", enter_handler},
        {"actionViewItemSelected", item_handler},
      });
}

void InputAndListViewSetItems(UserInterface& ui,
                              const QVector<QVariantList>& list_items) {
  ui.GetListField(kViewSlot, "dataViewListModel").SetItems(list_items);
}

void InputAndListViewSetInput(UserInterface& ui, const QString& value) {
  ui.SetDataField(kViewSlot, "dataViewInputValue", value);
}

void InputAndListViewSetButtonText(UserInterface& ui, const QString& value) {
  ui.SetDataField(kViewSlot, "dataViewButtonText", value);
}
