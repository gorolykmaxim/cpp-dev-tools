#include "InputAndListView.hpp"
#include <functional>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QList>
#include <QHash>
#include "UserInterface.hpp"

void InputAndListViewDisplay(
    const QString& input_label, const QString& input_value,
    const QString& button_text, const QVector<QVariantList>& list_items,
    const std::function<void(const QString&)>& on_input_value_changed,
    const std::function<void()>& on_enter_pressed,
    const std::function<void(int)>& on_item_selected, UserInterface& ui) {
  QList<DataField> data_fields = {
      DataField{"dataInputLabel", input_label},
      DataField{"dataInputValue", input_value},
      DataField{"dataError", ""},
      DataField{"dataButtonText", button_text}};
  QList<ListField> list_fields = {
      ListField{"dataListModel", {{0, "title"}}, list_items}};
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
  ui.GetListField("dataListModel").SetItems(list_items);
}

void InputAndListViewSetInput(const QString& value, UserInterface& ui) {
  ui.SetDataField("dataInputValue", value);
}

void InputAndListViewSetButtonText(const QString& value, UserInterface& ui) {
  ui.SetDataField("dataButtonText", value);
}
