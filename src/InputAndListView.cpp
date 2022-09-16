#include "InputAndListView.hpp"

void InputAndListViewDisplay(
    const QString& input_label, const QString& input_value,
    const QString& button_text, const QVector<QVariantList>& list_items,
    const std::function<void(const QString&)>& on_input_value_changed,
    const std::function<void()>& on_enter_pressed,
    const std::function<void(int)>& on_item_selected, UserInterface& ui) {
  QList<DataField> data_fields = {
      DataField{"inputAndListDataInputLabel", input_label},
      DataField{"inputAndListDataInputValue", input_value},
      DataField{"inputAndListDataError", ""},
      DataField{"inputAndListDataButtonText", button_text}};
  QList<ListField> list_fields = {
      ListField{"inputAndListDataListModel", {{0, "title"}}, list_items}};
  QHash<QString, UserActionHandler> user_action_handlers = {
      {"inputValueChanged", [on_input_value_changed] (const QVariantList& args)
      {
        on_input_value_changed(args[0].toString());
      }},
      {"enterPressed", [on_enter_pressed] (const QVariantList&) {
        on_enter_pressed();
      }},
      {"itemSelected", [on_item_selected] (const QVariantList& args) {
        on_item_selected(args[0].toInt());
      }}};
  ui.DisplayView("InputAndListView.qml", data_fields, list_fields,
                 user_action_handlers);
}

void InputAndListViewSetItems(const QVector<QVariantList>& list_items,
                              UserInterface& ui) {
  ui.GetListField("inputAndListDataListModel").SetItems(list_items);
}

void InputAndListViewSetInput(const QString& value, UserInterface& ui) {
  ui.SetDataField("inputAndListDataInputValue", value);
}

void InputAndListViewSetButtonText(const QString& value, UserInterface& ui) {
  ui.SetDataField("inputAndListDataButtonText", value);
}
