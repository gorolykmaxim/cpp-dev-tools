#include "OpenProject.hpp"
#include <QString>
#include <QVector>
#include <QVariant>
#include <QStandardPaths>

OpenProject::OpenProject() {
  EXEC_NEXT(DisplayOpenProjectView);
}

void OpenProject::DisplayOpenProjectView(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  app.ui.input_and_list.SetValue(home);
  QVector<QVariantList> items;
  items.append({"../"});
  items.append({"foo/"});
  items.append({"bar/"});
  items.append({"baz"});
  items.append({"tasks.json"});
  app.ui.input_and_list.list_model.SetItems(items);
  app.ui.input_and_list.on_value_changed = [] (const QString& value) {
    qDebug() << "new value" << value;
  };
  app.ui.input_and_list.on_list_item_selected = [] (int index) {
    qDebug() << "new item" << index;
  };
  app.ui.input_and_list.on_enter = [] () {
    qDebug() << "enter pressed";
  };
  app.ui.input_and_list.Display("Open project by path:", "Open");
}
