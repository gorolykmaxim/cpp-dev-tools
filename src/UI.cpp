#include "UI.hpp"

void InitializeUI(AppData& app) {
  QQmlContext* context = app.gui_engine.rootContext();
  QList<UIDataField> fields = {
    UIDataField{kViewSlot, ""},
    UIDataField{kDialogSlot, ""},
    UIDataField{kDialogVisible, false},
    UIDataField{kWindowTitle, "CPP Dev-Tools"},
  };
  DisplayMenuBar(app);
  DisplayStatusBar(app);
  context->setContextProperties(fields);
  context->setContextProperty("core", &app.ui_action_router);
  QQuickStyle::setStyle("Basic");
  app.gui_engine.load(QUrl(QStringLiteral("qrc:/cdt/qml/main.qml")));
}

void DisplayView(AppData& app, const QString& slot_name,
                 const QString& qml_file, const QList<UIDataField>& data_fields,
                 const QList<UIListField>& list_fields) {
  QQmlContext* context = app.gui_engine.rootContext();
  ViewData& data = app.view_data[slot_name];
  for (const QString& event_type: data.event_types) {
    qDebug() << "Removing listeners of event" << event_type;
    app.event_listeners.remove(event_type);
  }
  data.event_types.clear();
  // Initialize list_fields
  for (const QString& name: data.list_fields.keys()) {
    context->setContextProperty(name, nullptr);
  }
  data.list_fields.clear();
  for (const UIListField& list_field: list_fields) {
    auto model = QPtr<QVariantListModel>::create(list_field.role_names);
    model->SetItems(list_field.items);
    data.list_fields.insert(list_field.name, model);
    context->setContextProperty(list_field.name, model.get());
  }
  // Initialize data_fields and change current view
  QList<UIDataField> fields = data_fields;
  QSet<QString> data_field_names;
  for (const UIDataField& data_field: fields) {
    data_field_names.insert(data_field.name);
  }
  for (const QString& name: data.data_field_names) {
    if (!data_field_names.contains(name)) {
      fields.append(UIDataField{name, QVariant()});
    }
  }
  fields.append(UIDataField{slot_name, qml_file});
  data.data_field_names = data_field_names;
  context->setContextProperties(fields);
}

void SetUIDataField(AppData& app, const QString& slot_name,
                    const QString& name, const QVariant& value) {
  app.view_data[slot_name].data_field_names.insert(name);
  app.gui_engine.rootContext()->setContextProperty(name, value);
}

QVariantListModel& GetUIListField(AppData& app, const QString& slot_name,
                                  const QString& name) {
  ViewData& data = app.view_data[slot_name];
  Q_ASSERT(data.list_fields.contains(name));
  return *data.list_fields[name];
}

void DisplayAlertDialog(AppData& app, const QString& title,
                        const QString& text, bool error, bool cancellable) {
  DisplayView(
      app,
      kDialogSlot,
      "AlertDialog.qml",
      {
        UIDataField{kDialogVisible, true},
        UIDataField{"dTitle", title},
        UIDataField{"dText", text},
        UIDataField{"dCancellable", cancellable},
        UIDataField{"dError", error},
      },
      {});
}

void DisplayStatusBar(AppData& app) {
  QList<QVariantList> itemsLeft, itemsRight;
  if (app.projects.isEmpty()) {
    // Display empty status bar but make sure its 100% of its normal non-empty
    // height
    itemsLeft.append({" "});
  } else {
    const Project& project = app.projects[0];
    itemsLeft.append({project.GetPathRelativeToHome()});
    if (project.profile >= 0) {
      QString profile_name = app.profiles[project.profile].GetName();
      itemsRight.append({"Profile: " + profile_name});
    }
  }
  DisplayView(
      app,
      kStatusSlot,
      "",
      {},
      {
        UIListField{"sItemsLeft", {{0, "title"}}, itemsLeft},
        UIListField{"sItemsRight", {{0, "title"}}, itemsRight},
      });
}

void DisplayMenuBar(AppData& app) {
  QHash<int, QByteArray> role_names = {
      {0, "menu"},
      {1, "item"},
      {2, "shortcut"},
      {3, "eventType"}
  };
  QList<QVariantList> actions;
  for (const QString& event_type: app.user_commands.keys()) {
    UserCommand& cmd = app.user_commands[event_type];
    actions.append({cmd.group, cmd.name, cmd.shortcut, event_type});
  }
  DisplayView(
      app,
      kHeaderSlot,
      "",
      {},
      {
        UIListField{"hActions", role_names, actions},
      });
}
