#include "UserInterface.hpp"
#include <QtGlobal>
#include <QMetaObject>
#include "Dialog.hpp"

UserInterface::UserInterface() {
  engine.rootContext()->setContextProperty(kQmlCurrentView, "");
  engine.rootContext()->setContextProperty("core", this);
  Dialog::Init(*this);
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
  // In some cases this might get called WHILE we are in the middle
  // of the setContextProperties(), which can break some clients that
  // expect user action callbacks to always be called asynchronously.
  // This is why we are invoking it later instead of immediately right here.
  QMetaObject::invokeMethod(&engine, [this, action, args] () {
    UserActionHandler& handler = user_action_handlers[action];
    if (handler) {
      handler(args);
    }
  }, Qt::QueuedConnection);
}
