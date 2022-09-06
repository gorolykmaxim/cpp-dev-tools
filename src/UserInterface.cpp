#include "UserInterface.hpp"
#include <QtGlobal>

UserInterface::UserInterface() {
  engine.rootContext()->setContextProperty(kQmlCurrentView, "");
  engine.rootContext()->setContextProperty("core", this);
  engine.load(QUrl(QStringLiteral("qrc:/cdt/qml/main.qml")));
}

void UserInterface::DisplayView(
    const QString& qml_file,
    const QList<DataField>& data_fields,
    const QList<ListField>& list_fields,
    const QHash<QString, UserActionHandler> user_action_handlers) {
  QQmlContext* context = engine.rootContext();
  this->user_action_handlers = user_action_handlers;
  // Initialize list_fields
  for (QString& name: this->list_fields.keys()) {
    context->setContextProperty(name, nullptr);
  }
  this->list_fields.clear();
  for (const ListField& list_field: list_fields) {
    QPtr<QVariantListModel> model = QPtr<QVariantListModel>::create(
        list_field.role_names);
    model->SetItems(list_field.items);
    this->list_fields.insert(list_field.name, model);
    context->setContextProperty(list_field.name, model.get());
  }
  // Initialize data_fields and change current view
  QList<DataField> fields = data_fields;
  QSet<QString> new_data_field_names;
  for (const DataField& data_field: data_fields) {
    new_data_field_names.insert(data_field.name);
  }
  for (const QString& name: data_field_names) {
    if (new_data_field_names.contains(name)) {
      continue;
    }
    fields.append(DataField{name, QVariant()});
  }
  fields.append(DataField{kQmlCurrentView, qml_file});
  data_field_names = new_data_field_names;
  context->setContextProperties(fields);
}

void UserInterface::SetDataField(const QString& name, const QVariant& value) {
  data_field_names.insert(name);
  engine.rootContext()->setContextProperty(name, value);
}

QVariantListModel& UserInterface::GetListField(const QString& name) {
  Q_ASSERT(list_fields.contains(name));
  return *list_fields[name];
}

void UserInterface::OnUserAction(const QString& action,
                                 const QVariantList& args) {
  UserActionHandler& handler = user_action_handlers[action];
  if (handler) {
    handler(args);
  }
}

void InputAndListView::Display(
    const QString& input_label, const QString& input_value,
    const QString& button_text, const QVector<QVariantList>& list_items,
    const std::function<void(const QString&)>& on_input_value_changed,
    const std::function<void()>& on_enter_pressed,
    const std::function<void(int)>& on_item_selected, UserInterface& ui) {
  QList<DataField> data_fields = {
      DataField{"inputAndListDataInputLabel", input_label},
      DataField{"inputAndListDataInputValue", input_value},
      DataField{"inputAndListDataButtonText", button_text},
      DataField{"inputAndListDataIsButtonEnabled", false}};
  QList<ListField> list_fields = {
      ListField{"inputAndListDataListModel", {{0, "title"}}, list_items}};
  QHash<QString, UserActionHandler> user_action_handlers = {
      {"inputValueChanged", [on_input_value_changed] (const QVariantList& args)
      {
        on_input_value_changed(args[0].toString());
      }},
      {"enterPressed", [on_enter_pressed] (const QVariantList& args) {
        on_enter_pressed();
      }},
      {"itemSelected", [on_item_selected] (const QVariantList& args) {
        on_item_selected(args[0].toInt());
      }}};
  ui.DisplayView("InputAndListView.qml", data_fields, list_fields,
                 user_action_handlers);
}
