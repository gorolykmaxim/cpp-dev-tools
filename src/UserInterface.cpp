#include "UserInterface.hpp"

QVariantListModel::QVariantListModel(const QHash<int, QByteArray>& role_names)
    : QAbstractListModel(), role_names(role_names) {}

int QVariantListModel::rowCount(const QModelIndex&) const {
  return items.size();
}

QHash<int, QByteArray> QVariantListModel::roleNames() const {
  return role_names;
}

QVariant QVariantListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  if (index.row() < 0 || index.row() >= items.size()) {
    return QVariant();
  }
  const QVariantList& row = items[index.row()];
  Q_ASSERT(role >= 0 && role < row.size());
  return row[role];
}

void QVariantListModel::SetItems(const QVector<QVariantList>& items) {
  beginResetModel();
  this->items = items;
  endResetModel();
}

UserInterface::UserInterface() {
  QList<DataField> fields = {
    DataField{kViewSlot, ""},
    DataField{kDialogSlot, ""},
    DataField{kDataDialogVisible, false},
    DataField{kDataWindowTitle, "CPP Dev-Tools"},
  };
  engine.rootContext()->setContextProperties(fields);
  engine.rootContext()->setContextProperty("core", this);
  QQuickStyle::setStyle("Basic");
  engine.load(QUrl(QStringLiteral("qrc:/cdt/qml/main.qml")));
}

void UserInterface::DisplayView(
      const QString& slot_name, const QString& qml_file,
      const QList<DataField>& data_fields, const QList<ListField>& list_fields,
      const QHash<QString, UserActionHandler>& user_action_handlers) {
  QQmlContext* context = engine.rootContext();
  ViewData& data = view_slot_name_to_data[slot_name];
  data.user_action_handlers = user_action_handlers;
  // Initialize list_fields
  for (const QString& name: data.list_fields.keys()) {
    context->setContextProperty(name, nullptr);
  }
  data.list_fields.clear();
  for (const ListField& list_field: list_fields) {
    auto model = QPtr<QVariantListModel>::create(list_field.role_names);
    model->SetItems(list_field.items);
    data.list_fields.insert(list_field.name, model);
    context->setContextProperty(list_field.name, model.get());
  }
  // Initialize data_fields and change current view
  QList<DataField> fields = data_fields;
  QSet<QString> data_field_names;
  for (const DataField& data_field: fields) {
    data_field_names.insert(data_field.name);
  }
  for (const QString& name: data.data_field_names) {
    if (!data_field_names.contains(name)) {
      fields.append(DataField{name, QVariant()});
    }
  }
  fields.append(DataField{slot_name, qml_file});
  data.data_field_names = data_field_names;
  context->setContextProperties(fields);
}

void UserInterface::SetDataField(const QString& slot_name, const QString& name,
                                 const QVariant& value) {
  view_slot_name_to_data[slot_name].data_field_names.insert(name);
  engine.rootContext()->setContextProperty(name, value);
}

QVariantListModel& UserInterface::GetListField(const QString& slot_name,
                                               const QString& name) {
  ViewData& data = view_slot_name_to_data[slot_name];
  Q_ASSERT(data.list_fields.contains(name));
  return *data.list_fields[name];
}

void UserInterface::DisplayInputAndListView(
    const QString& title, const QString& input_value,
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
  DisplayView(
      kViewSlot,
      "InputAndListView.qml",
      {
        DataField{kDataWindowTitle, title},
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

void UserInterface::SetInputAndListViewItems(
    const QVector<QVariantList>& list_items) {
  GetListField(kViewSlot, "dataViewListModel").SetItems(list_items);
}

void UserInterface::SetInputAndListViewInput(const QString& value) {
  SetDataField(kViewSlot, "dataViewInputValue", value);
}

void UserInterface::SetInputAndListViewButtonText(const QString& value) {
  SetDataField(kViewSlot, "dataViewButtonText", value);
}

void UserInterface::DisplayAlertDialog(
    const QString& title, const QString& text, bool error, bool cancellable,
    const std::function<void()>& on_ok,
    const std::function<void()>& on_cancel) {
  UserActionHandler ok_handler = [on_ok] (const QVariantList&) {
    if (on_ok) {
      on_ok();
    }
  };
  UserActionHandler cancel_handler = [on_cancel] (const QVariantList&) {
    if (on_cancel) {
      on_cancel();
    }
  };
  DisplayView(
      kDialogSlot,
      "AlertDialog.qml",
      {
        DataField{kDataDialogVisible, true},
        DataField{"dataDialogTitle", title},
        DataField{"dataDialogText", text},
        DataField{"dataDialogCancellable", cancellable},
        DataField{"dataDialogError", error},
      },
      {},
      {
        {"actionDialogOk", ok_handler},
        {"actionDialogCancel", cancel_handler},
      });
}

void UserInterface::OnUserAction(const QString& slot_name,
                                 const QString& action,
                                 const QVariantList& args) {
  // In some cases this might get called WHILE we are in the middle
  // of the setContextProperties(), which can break some clients that
  // expect user action callbacks to always be called asynchronously.
  // This is why we are invoking it later instead of immediately right here.
  QMetaObject::invokeMethod(&engine, [this, slot_name, action, args] () {
    ViewData& data = view_slot_name_to_data[slot_name];
    UserActionHandler& handler = data.user_action_handlers[action];
    if (handler) {
      handler(args);
    }
  }, Qt::QueuedConnection);
}
