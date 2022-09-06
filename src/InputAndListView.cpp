#include "InputAndListView.hpp"
#include "Common.hpp"

InputAndListView::InputAndListView(QQmlContext* context)
    : context(context),
      list_model({{0, "title"}}) {
  context->setContextProperty("inputAndListData", this);
}

void InputAndListView::OnEnter() {
  if (on_enter) {
    on_enter();
  }
}

void InputAndListView::OnListItemSelected(int index) {
  if (on_list_item_selected) {
    on_list_item_selected(index);
  }
}

void InputAndListView::OnValueChanged(const QString& value) {
  if (on_value_changed) {
    on_value_changed(value);
  }
}

void InputAndListView::Reset() {
  input_value = "";
  input_label = "";
  button_text = "";
  is_button_enabled = false;
  on_enter = nullptr;
  on_value_changed = nullptr;
  on_list_item_selected = nullptr;
  list_model.SetItems(QVector<QVariantList>());
}

void InputAndListView::Display(const QString& input_label,
                               const QString& button_text) {
  this->input_label = input_label;
  this->button_text = button_text;
  emit LabelsChanged();
  context->setContextProperty(kQmlCurrentView, "InputAndListView.qml");
}

void InputAndListView::SetValue(const QString& value) {
  input_value = value;
  emit InputValueChanged();
}

void InputAndListView::SetButtonEnabled(bool value) {
  is_button_enabled = value;
  emit IsButtonEnabledChanged();
}

QVariantListModel* InputAndListView::GetListModel() {
  return &list_model;
}
