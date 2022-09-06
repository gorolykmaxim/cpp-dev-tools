#include "OpenProject.hpp"
#include <QString>
#include <QVector>
#include <QVariant>
#include <QStandardPaths>
#include "InputAndListView.hpp"

OpenProject::OpenProject() {
  EXEC_NEXT(DisplayOpenProjectView);
}

void OpenProject::DisplayOpenProjectView(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QVector<QVariantList> items;
  items.append({"../"});
  items.append({"foo/"});
  items.append({"bar/"});
  items.append({"baz"});
  items.append({"tasks.json"});
  std::function<void(const QString&)> on_new_value = [] (const QString& value) {
    qDebug() << "new value" << value;
  };
  std::function<void()> on_enter = [] () {
    qDebug() << "enter pressed";
  };
  std::function<void(int)> on_item_selected = [] (int item) {
    qDebug() << "new item" << item;
  };
  InputAndListView::Display("Open project by path:", home, "Open", items,
                            on_new_value, on_enter, on_item_selected,
                            app.ui);
}
