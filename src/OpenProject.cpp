#include "OpenProject.hpp"
#include <QString>
#include <QVector>
#include <QVariant>
#include <QList>
#include <QHash>
#include <QStandardPaths>

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
  QList<DataField> data_fields = {
      DataField{kQmlInputAndListDataInputLabel, "Open project by path:"},
      DataField{kQmlInputAndListDataInputValue, home},
      DataField{kQmlInputAndListDataButtonText, "Open"},
      DataField{kQmlInputAndListDataIsButtonEnabled, false}};
  QList<ListField> list_fields = {
      ListField{kQmlInputAndListDataListModel,
                kQmlInputAndListDataListModelRoles,
                items}};
  QHash<QString, UserActionHandler> user_action_handlers = {
      {kQmlInputAndListActionInputValueChanged, [] (const QVariantList& args) {
        qDebug() << "new value" << args[0];
      }},
      {kQmlInputAndListActionEnterPressed, [] (const QVariantList& args) {
        qDebug() << "enter pressed";
      }},
      {kQmlInputAndListActionItemSelected, [] (const QVariantList& args) {
        qDebug() << "new item" << args[0];
      }},
  };
  app.ui.DisplayView(kQmlInputAndListView, data_fields, list_fields,
                     user_action_handlers);
}
